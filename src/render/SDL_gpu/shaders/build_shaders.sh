#!/usr/bin/env bash
set -euo pipefail

# Builds SDL_gpu GLSL shaders to SPIR-V and embeds them as C headers.
# Requirements: glslangValidator, spirv-val, spirv-dis, spirv-as, python3
#
# Shader sources use named entry functions (e.g. NoteVS, KeyboardFS). The
# compiler requires main(), so we compile a temp copy with main() and then patch
# the SPIR-V OpEntryPoint name back to the original function name.

cd "$(dirname "$0")"

shaders=(
  "note_vs.glsl:vert:NoteVS:note_vs"
  "note_fs.glsl:frag:NoteFS:note_fs"
  "keyboard_vs.glsl:vert:KeyboardVS:keyboard_vs"
  "keyboard_fs.glsl:frag:KeyboardFS:keyboard_fs"
)

gen_header() {
  local spv="$1" hdr="$2" sym="$3"
  python3 - "$spv" "$hdr" "$sym" <<'PY'
from pathlib import Path
import sys
spv, hdr, sym = sys.argv[1], sys.argv[2], sys.argv[3]
data = Path(spv).read_bytes()
out = [f'unsigned char {sym}[] = {{\n']
for i in range(0, len(data), 12):
    chunk = data[i:i+12]
    out.append('  ' + ', '.join(f'0x{b:02x}' for b in chunk) + ',\n')
out.append('};\n')
out.append(f'unsigned int {sym}_len = {len(data)};\n')
Path(hdr).write_text(''.join(out))
print(f'  {hdr} ({len(data)} bytes)')
PY
}

for entry in "${shaders[@]}"; do
  IFS=: read -r src stage func base <<<"$entry"
  spv="${base}.spv"
  hdr="${base}.h"
  sym="${base}_raw"

  echo "==> $src"

  tmp="$(mktemp --suffix=.glsl)"
  trap 'rm -f "$tmp"' EXIT
  python3 - "$src" "$func" "$tmp" <<'PY'
from pathlib import Path
import sys
src, func, tmp = sys.argv[1], sys.argv[2], sys.argv[3]
text = Path(src).read_text()
old = f'void {func}()'
if old not in text:
    raise SystemExit(f'entry function {old} not found in {src}')
Path(tmp).write_text(text.replace(old, 'void main()', 1))
PY

  glslangValidator -V --target-env vulkan1.0 -S "$stage" -o "$spv" "$tmp"
  # Restore original entry point name in SPIR-V and normalize SSBO decorations
  # for Vulkan (glslang may emit BufferBlock/Uniform for storage buffers).
  spirv-dis "$spv" -o "$spv.dis"
  python3 - "$spv.dis" "$func" <<'PY'
from pathlib import Path
import re, sys

p, func = Path(sys.argv[1]), sys.argv[2]
text = p.read_text()
text = text.replace('OpEntryPoint Vertex %main "main"', f'OpEntryPoint Vertex %main "{func}"', 1)
text = text.replace('OpEntryPoint Fragment %main "main"', f'OpEntryPoint Fragment %main "{func}"', 1)

ssbo_structs = set(re.findall(r'OpDecorate\s+(%\S+)\s+BufferBlock\b', text))
if ssbo_structs:
    text = re.sub(r'\bBufferBlock\b', 'Block', text)
    if 'SPV_KHR_storage_buffer_storage_class' not in text:
        lines = text.split('\n')
        ins = 0
        for i, l in enumerate(lines):
            if re.match(r'\s*Op(Capability|Extension)\b', l):
                ins = i
        lines.insert(ins + 1, '               OpExtension "SPV_KHR_storage_buffer_storage_class"')
        text = '\n'.join(lines)

    type_def = {}
    for m in re.finditer(r'^\s*(%\S+)\s*=\s*(OpType\w+)\s*(.*)$', text, re.M):
        type_def[m.group(1)] = (m.group(2), re.findall(r'%\S+', m.group(3)))

    reachable = set(ssbo_structs)
    changed = True
    while changed:
        changed = False
        for tid in list(reachable):
            for r in type_def.get(tid, (None, []))[1]:
                if r not in reachable and r in type_def:
                    reachable.add(r)
                    changed = True

    uniform_ptrs = {}
    for m in re.finditer(r'^\s*(%\S+)\s*=\s*OpTypePointer\s+Uniform\s+(%\S+)\s*$', text, re.M):
        if m.group(2) in reachable:
            uniform_ptrs[m.group(1)] = m.group(2)

    ssbo_vars = set()
    var_ptr = {}
    for m in re.finditer(r'^\s*(%\S+)\s*=\s*OpVariable\s+(%\S+)\s+Uniform\b', text, re.M):
        var_ptr[m.group(1)] = m.group(2)
        if m.group(2) in uniform_ptrs:
            ssbo_vars.add(m.group(1))

    chain_info = []
    for m in re.finditer(r'^\s*(%\S+)\s*=\s*(Op(?:InBounds)?AccessChain)\s+(%\S+)\s+(%\S+)', text, re.M):
        chain_info.append((m.group(1), m.group(2), m.group(3), m.group(4)))

    ssbo_derived = set(ssbo_vars)
    changed = True
    while changed:
        changed = False
        for res, _op, _ptype, base in chain_info:
            if base in ssbo_derived and res not in ssbo_derived:
                ssbo_derived.add(res)
                changed = True

    ssbo_use, nonssbo_use = set(), set()
    for v, pt in var_ptr.items():
        if pt in uniform_ptrs:
            (ssbo_use if v in ssbo_vars else nonssbo_use).add(pt)
    for _res, _op, ptype, base in chain_info:
        if ptype in uniform_ptrs:
            (ssbo_use if base in ssbo_derived else nonssbo_use).add(ptype)

    need_split = ssbo_use & nonssbo_use
    only_ssbo = ssbo_use - nonssbo_use

    ids = [int(x) for x in re.findall(r'%(\d+)', text)]
    next_id = max(ids) + 1 if ids else 1000
    bound_m = re.search(r'\bOpBound\s+(\d+)', text)
    bound = int(bound_m.group(1)) if bound_m else next_id

    new_types = {}
    for pt in sorted(need_split):
        new_types[pt] = f'%{next_id}'
        next_id += 1

    if new_types:
        lines = text.split('\n')
        insert_at = 0
        for i, l in enumerate(lines):
            if re.search(r'\bOpTypePointer\b', l):
                insert_at = i
        extra = [f'         {new} = OpTypePointer StorageBuffer {uniform_ptrs[old]}' for old, new in new_types.items()]
        for j, l in enumerate(extra):
            lines.insert(insert_at + 1 + j, l)
        text = '\n'.join(lines)
        bound = max(bound, next_id)

    for pt in only_ssbo:
        pointee = uniform_ptrs[pt]
        text = re.sub(
            rf'^(\s*{re.escape(pt)}\s*=\s*OpTypePointer\s+)Uniform(\s+{re.escape(pointee)}\s*)$',
            r'\g<1>StorageBuffer\g<2>', text, flags=re.M)

    for v in ssbo_vars:
        pt = var_ptr[v]
        new_pt = new_types.get(pt, pt)
        text = re.sub(
            rf'^(\s*{re.escape(v)}\s*=\s*OpVariable\s+){re.escape(pt)}(\s+)Uniform\b',
            rf'\g<1>{new_pt}\g<2>StorageBuffer', text, flags=re.M)

    for res, _op, ptype, base in chain_info:
        if base in ssbo_derived and ptype in new_types:
            text = re.sub(
                rf'^(\s*{re.escape(res)}\s*=\s*Op(?:InBounds)?AccessChain\s+){re.escape(ptype)}(\s+)',
                rf'\g<1>{new_types[ptype]}\g<2>', text, flags=re.M)

    text = re.sub(r'\bOpBound\s+\d+', f'OpBound {bound}', text)

p.write_text(text)
PY
  spirv-as "$spv.dis" -o "$spv"
  rm -f "$spv.dis"
  spirv-val "$spv"
  gen_header "$spv" "$hdr" "$sym"
  rm -f "$tmp"
  trap - EXIT
done

echo "All shaders built."