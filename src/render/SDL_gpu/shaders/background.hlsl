cbuffer BgUniforms : register(b0, space1)
{
    float4 clearColor;
    float winW, winH, tplay, pps;
};

struct VSOut { float4 pos : SV_Position; float2 uv : TEXCOORD0; };

VSOut BgVS(uint vid : SV_VertexID)
{
    float2 q = float2((vid << 1) & 2, vid & 2);
    VSOut o;
    o.pos = float4(q * 2.0 - 1.0, 0, 1);
    o.uv  = q;
    return o;
}

float4 BgFS(VSOut i) : SV_Target {
    return clearColor;
}