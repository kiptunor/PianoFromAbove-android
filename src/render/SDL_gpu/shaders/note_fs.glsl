#version 450
layout(location=0) in vec4 inColor;
layout(location=1) in vec2 inLocal;
layout(location=0) out vec4 outColor;
void NoteFS() {
    vec4 c = inColor;
    vec2 d = min(inLocal, 1.0 - inLocal);
    float edge = min(d.x, d.y);
    float border = smoothstep(0.0, 0.03, edge);
    vec3 dark = c.rgb * 0.2;
    vec3 fill = mix(c.rgb * 0.6, c.rgb, inLocal.x * (1.0 - inLocal.y) + 0.2);
    outColor = vec4(mix(dark, fill, border), c.a);
}