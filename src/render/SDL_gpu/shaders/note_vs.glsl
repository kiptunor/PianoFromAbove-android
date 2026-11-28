#version 450

layout(location=0) out vec4 outColor;
layout(location=1) out vec2 outLocal;

struct GpuNote {
    float start;
    float noteLength;
    uint key;
    uint colorIndexFlags;
};

layout(std430, set=0, binding=0) readonly buffer NotesBuf {
    GpuNote notes[];
};

layout(std430, set=0, binding=1) readonly buffer KeyGeomBuf {
    vec4 keyGeom[]; // x, w, isSharp, pad
};

layout(std140, set=1, binding=0) uniform NoteUniforms {
    float tplay;
    float tscr;
    float winW;
    float winH;
    float pps;
    float kbHeight;
    vec2 pad;
    vec4 colors[16];
} u;

vec2 quadVert(int vid) {
    if (vid == 0) return vec2(0.0, 0.0);
    if (vid == 1) return vec2(1.0, 0.0);
    if (vid == 2) return vec2(1.0, 1.0);
    if (vid == 3) return vec2(0.0, 0.0);
    if (vid == 4) return vec2(1.0, 1.0);
    return vec2(0.0, 1.0);
}

void NoteVS() {
    uint vid = uint(gl_VertexIndex % 6);
    uint iid = uint(gl_InstanceIndex);
    GpuNote n = notes[iid];
    vec2 q = quadVert(int(vid));

    float start = n.start;
    float endT = n.start + n.noteLength;
    float kbTop = u.winH - u.kbHeight;
    float y0 = kbTop - (start - u.tplay) * u.pps; // bottom
    float y1 = kbTop - (endT  - u.tplay) * u.pps; // top

    vec4 kg = keyGeom[n.key];
    float x = kg.x;
    float w = kg.y;

    if (endT <= u.tplay || y1 >= kbTop || start > u.tplay + u.tscr) {
        gl_Position = vec4(2.0, 2.0, 0.0, 1.0);
        outColor = vec4(0.0);
        outLocal = vec2(0.0);
        return;
    }

    float yb = min(y0, kbTop);
    float px = x + q.x * w;
    float py = mix(y1, yb, q.y); // top->bottom, clipped at keyboard top

    float ndcX = (px / u.winW) * 2.0 - 1.0;
    float ndcY = 1.0 - (py / u.winH) * 2.0;

    gl_Position = vec4(ndcX, ndcY, 0.0, 1.0);
    uint ci = n.colorIndexFlags & 0xFFu;
    outColor = u.colors[ci % 16u];
    outLocal = q;
}