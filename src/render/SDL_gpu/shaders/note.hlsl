struct GpuNote {
    float start;
    float length;
    uint key;
    uint colorIndexFlags; // pack key/color/flags as needed; or use StructuredBuffer<uint>
};

StructuredBuffer<GpuNote> Notes : register(t0, space0);

cbuffer NoteUniforms : register(b0, space1) {
    float tplay;
    float tscr;
    float winW;
    float winH;
    float pps;
    float kbHeight;
    float2 pad;
    float4 colors[16];
};

struct VSOut {
    float4 pos : SV_Position;
    float4 color : TEXCOORD0;
    float2 local : TEXCOORD1;
};

static const float2 Quad[6] = {
    float2(0,0), float2(1,0), float2(1,1),
    float2(0,0), float2(1,1), float2(0,1)
};

// Key X layout should match SDLRenderer::KeyX/_KeyWidth. Pass as uniform/storage if dynamic.
// For first pass, pass precomputed key x/width arrays in another storage buffer or uniform.

StructuredBuffer<float4> KeyGeom : register(t1, space0); // x, w, isSharp, pad

VSOut NoteVS(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
    GpuNote n = Notes[iid];
    float2 q = Quad[vid];

    float start = n.start;
    float end = n.start + n.length;
    float y0 = winH - (start - tplay) * pps; // bottom
    float y1 = winH - (end   - tplay) * pps; // top

    float4 kg = KeyGeom[n.key]; // or KeyMap/key index mapping
    float x = kg.x;
    float w = kg.y;

    float px = x + q.x * w;
    float py = lerp(y1, y0, q.y); // top->bottom

    // Clip invisible notes cheaply in VS by collapsing; better: CPU/compute culling.
    if(end < tplay || start > tplay + tscr)
    {
        VSOut o; o.pos = float4(2,2,0,1); o.color = 0; o.local = 0; return o;
    }

    float2 ndc;
    ndc.x = (px / winW) * 2.0 - 1.0;
    ndc.y = 1.0 - (py / (winH - kbHeight)) * 2.0; // adjust to note area

    VSOut o;
    o.pos = float4(ndc, 0, 1);
    uint ci = n.colorIndexFlags & 0xFF;
    o.color = colors[ci % 16];
    o.local = q;
    return o;
}

float4 NoteFS(VSOut i) : SV_Target
{
    // PFA-style border/gradient like SDLRenderer::CreateNote
    float4 c = i.color;
    float2 d = min(i.local, 1.0 - i.local);
    float edge = min(d.x, d.y);
    float border = smoothstep(0.0, 0.03, edge);
    float3 dark = c.rgb * 0.2;
    float3 fill = lerp(c.rgb * 0.6, c.rgb, i.local.x * (1.0 - i.local.y) + 0.2);
    return float4(lerp(dark, fill, border), c.a);
}