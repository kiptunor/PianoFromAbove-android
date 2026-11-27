#version 450
layout(location=0) in vec4 inColor;
layout(location=1) in vec2 inLocal;
layout(location=0) out vec4 outColor;
void NoteFS() {
    /*
    vec4 c = inColor;
    vec2 d = min(inLocal, 1.0 - inLocal);
    float edge = min(d.x, d.y);
    float border = smoothstep(0.0, 0.03, edge);
    vec3 dark = c.rgb * 0.2;
    vec3 fill = mix(c.rgb * 0.6, c.rgb, inLocal.x * (1.0 - inLocal.y) + 0.2);
    outColor = vec4(mix(dark, fill, border), c.a);
    */

    vec4 c = inColor;
    
    // Solid border thickness in pixels, no gradient.
    const float borderPx = 2.0;
    vec2 fw = fwidth(inLocal);
    vec2 d = min(inLocal, 1.0 - inLocal);
    vec2 dPx = d / max(fw, vec2(1e-5));
    float edgePx = min(dPx.x, dPx.y);

    vec3 darker = c.rgb * 0.2;
    // Horizontal gradient: left=primary, right=lighter (60%)
    vec3 lighter = c.rgb * 0.6;
    vec3 fill = mix(c.rgb, lighter, inLocal.x);

    vec3 rgb = (edgePx < borderPx) ? darker : fill;
    outColor = vec4(rgb, c.a);
}