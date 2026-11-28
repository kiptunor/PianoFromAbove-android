#version 450

layout(location=0) out vec4 outColor;

struct KeyState {
    uint pressed;
    uint color;   // 0xAABBGGRR
    uint pad0;
    uint pad1;
};

layout(std430, set=0, binding=0) readonly buffer KeyStateBuf {
    KeyState keyState[128];
};

layout(std140, set=1, binding=0) uniform KbUniforms {
    float winW;
    float winH;
    vec2 pad;
} u;

const int GenKeyX[12] = int[12](0,12,18,33,36,54,66,72,85,90,105,108);
const int KeyMapArr[128] = int[128](
    0,2,4,5,7,9,11,12,14,16,17,19,21,23,24,26,
    28,29,31,33,35,36,38,40,41,43,45,47,48,50,52,53,
    55,57,59,60,62,64,65,67,69,71,72,74,76,77,79,81,
    83,84,86,88,89,91,93,95,96,98,100,101,103,105,107,108,
    110,112,113,115,117,119,120,122,124,125,127,1,3,6,8,10,
    13,15,18,20,22,25,27,30,32,34,37,39,42,44,46,49,
    51,54,56,58,61,63,66,68,70,73,75,78,80,82,85,87,
    90,92,94,97,99,102,104,106,109,111,114,116,118,121,123,126
);

vec4 unpackABGR(uint c) {
    return vec4(
        float(c & 0xFFu),
        float((c >> 8u) & 0xFFu),
        float((c >> 16u) & 0xFFu),
        float((c >> 24u) & 0xFFu)
    ) / 255.0;
}

uint packABGR(vec4 c) {
    uvec4 v = uvec4(clamp(c * 255.0 + 0.5, 0.0, 255.0));
    return v.r | (v.g << 8u) | (v.b << 16u) | (v.a << 24u);
}

vec2 corner(int vid) {
    // two triangles: 0,1,2, 0,2,3
    if (vid == 0 || vid == 3) return vec2(0.0, 0.0);
    if (vid == 1) return vec2(1.0, 0.0);
    if (vid == 2 || vid == 4) return vec2(1.0, 1.0);
    return vec2(0.0, 1.0);
}

void emit(vec2 p, vec4 c) {
    float ndcX = (p.x / u.winW) * 2.0 - 1.0;
    float ndcY = 1.0 - (p.y / u.winH) * 2.0;
    gl_Position = vec4(ndcX, ndcY, 0.0, 1.0);
    outColor = c;
}

void emitDegenerate() {
    gl_Position = vec4(2.0, 2.0, 0.0, 1.0);
    outColor = vec4(0.0);
}

void emitRect(int vid, vec2 pos, vec2 size, uint cTL, uint cTR, uint cBR, uint cBL) {
    vec2 k = corner(vid);
    vec2 p = pos + k * size;
    vec4 c = mix(mix(unpackABGR(cTL), unpackABGR(cTR), k.x),
                 mix(unpackABGR(cBL), unpackABGR(cBR), k.x), k.y);
    emit(p, c);
}

void emitSkew(int vid, vec2 p1, vec2 p2, vec2 p3, vec2 p4, uint c1, uint c2, uint c3, uint c4) {
    vec2 p;
    vec4 c;
    if (vid == 0) { p=p1; c=unpackABGR(c1); }
    else if (vid == 1) { p=p2; c=unpackABGR(c2); }
    else if (vid == 2 || vid == 4) { p=p3; c=unpackABGR(c3); }
    else if (vid == 3) { p=p1; c=unpackABGR(c1); }
    else { p=p4; c=unpackABGR(c4); }
    emit(p, c);
}

int keyXAt(int midiKey, float winW) {
    return (midiKey / 12 * 126 + GenKeyX[midiKey % 12]) * int(winW) / 1350;
}

int keyWidthAt(int midiKey, float winW) {
    int kx = keyXAt(midiKey, winW);
    if (midiKey == 127) return int(winW) - kx;
    int n = midiKey % 12;
    int val;
    if (n==1||n==3||n==6||n==8||n==10) val = int(winW) * 9 / 1350;
    else if (n==4||n==11) val = keyXAt(midiKey+1, winW) - kx;
    else val = keyXAt(midiKey+2, winW) - kx;
    return val;
}

bool isSharpKey(int midiKey) {
    int n = midiKey % 12;
    return n==1||n==3||n==6||n==8||n==10;
}

void KeyboardVS() {
    int vid = gl_VertexIndex % 6;
    int prim = gl_VertexIndex / 6;

    float WinW = u.winW;
    float WinH = u.winH;
    float SharpRatio = 0.65f;

    float fTransitionPct = 0.02;
    float fRedPct = 0.05;
    float fSpacerCY = 2.0;
    float fRedCY = floor(WinW * 82.0 / 1000.0 * fRedPct + 0.5);
    float fTransitionCY = max(3.0, floor(WinW * 82.0 / 1000.0 * fTransitionPct + 0.5));
    float fTopCY = floor((WinW * 82.0 / 1000.0 - fSpacerCY - fRedCY - fTransitionCY) * 0.95 + 0.5);
    float fNearCY = WinW * 82.0 / 1000.0 - fSpacerCY - fRedCY - fTransitionCY - fTopCY;
    float kw0 = float(keyWidthAt(0, WinW));
    float fKeyGap = max(1.0, floor(kw0 * 0.05 + 0.5));
    float fKeyGap1 = fKeyGap - floor(fKeyGap / 2.0 + 0.5);
    float fCurYBase = fTransitionCY + fRedCY + fSpacerCY;
    float fSharpCY = fTopCY * 0.67;
    float kbBaseY = WinH - WinW * 82.0 / 1000.0;

    // 0: top gradient, 1: red bar, 2: spacer
    if (prim == 0) {
        float fTopGradCY = (WinW * 82.0 / 1000.0 - fTransitionCY - fRedCY - fSpacerCY) * 0.02;
        emitRect(vid, vec2(0.0, kbBaseY + fTransitionCY - fTopGradCY), vec2(WinW, fTopGradCY),
                 0x00333333u, 0x00333333u, 0xFF000000u, 0xFF000000u);
        return;
    }
    if (prim == 1) {
        emitRect(vid, vec2(0.0, kbBaseY + fTransitionCY), vec2(WinW, fRedCY),
                 0xFF06054Cu, 0xFF06054Cu, 0xFF0D0A98u, 0xFF0D0A98u);
        return;
    }
    if (prim == 2) {
        emitRect(vid, vec2(0.0, kbBaseY + fTransitionCY + fRedCY), vec2(WinW, fSpacerCY),
                 0xFF3D3D3Du, 0xFF3D3D3Du, 0xFF3D3D3Du, 0xFF3D3D3Du);
        return;
    }

    // White keys: 75 keys * 5 prims = 375 prims, starting at prim 3
    int whiteBase = 3;
    int whitePrimCount = 75 * 5;
    if (prim < whiteBase + whitePrimCount) {
        int wp = prim - whiteBase;
        int i = wp / 5;
        int kind = wp % 5; // 0 body, 1 near, 2 trans, 3 c60, 4 edge
        int j = KeyMapArr[i];

        // compute fCurX for this white key index by summing previous white key widths
        float fCurX = 0.0;
        for(int k = 0; k < 75; ++k) {
            if(k >= i) break;
            int jk = KeyMapArr[k];
            fCurX += float(keyWidthAt(jk, WinW));
        }

        float x = fCurX + fKeyGap1;
        float w = float(keyWidthAt(j, WinW)) - fKeyGap;
        float yTop = fCurYBase + kbBaseY;
        bool pressed = keyState[j].pressed != 0u;
        uint c = keyState[j].color;

        if (!pressed) {
            if (kind == 0) {
                emitRect(vid, vec2(x, yTop), vec2(w, fTopCY + fNearCY),
                         0xFFCCCCCCu, 0xFFCCCCCCu, 0xFFFFFFFFu, 0xFFFFFFFFu); return;
            }
            if (kind == 1) {
                emitRect(vid, vec2(x, fCurYBase + fTopCY + kbBaseY), vec2(w, fNearCY),
                         0xFFCCCCCCu, 0xFFCCCCCCu, 0xFF999999u, 0xFF999999u); return;
            }
            if (kind == 2) {
                emitRect(vid, vec2(x, fCurYBase + fTopCY + kbBaseY), vec2(w, 2.0),
                         0xFF3D3D3Du, 0xFF3D3D3Du, 0xFF999999u, 0xFF999999u); return;
            }
            if (kind == 3 && j == 60) {
                float fMXGap = floor(float(keyWidthAt(j, WinW)) * 0.25 + 0.5);
                float fMCX = float(keyWidthAt(j, WinW)) - fMXGap * 2.0 - fKeyGap;
                float fMY = max(fCurYBase + fTopCY - fMCX - 5.0, fCurYBase + fSharpCY + 5.0);
                emitRect(vid, vec2(fCurX + fKeyGap1 + fMXGap, fMY + kbBaseY),
                         vec2(fMCX, fCurYBase + fTopCY - 5.0 - fMY),
                         0xFFCCCCCCu, 0xFFCCCCCCu, 0xFFCCCCCCu, 0xFFCCCCCCu); return;
            }
        } else {
            uint r = (c >> 16u) & 0xFFu;
            uint g = (c >> 8u) & 0xFFu;
            uint b = c & 0xFFu;
            uint r2 = uint(float(r) * 0.6);
            uint g2 = uint(float(g) * 0.6);
            uint b2 = uint(float(b) * 0.6);
            uint c_bgr = 0xFF000000u | (b << 16u) | (g << 8u) | r;
            uint darker = 0xFF000000u | (b2 << 16u) | (g2 << 8u) | r2;
            if (kind == 0) {
                emitRect(vid, vec2(x, yTop), vec2(w, fTopCY + fNearCY - 2.0),
                         darker, darker, c_bgr, c_bgr); return;
            }
            if (kind == 1) {
                emitRect(vid, vec2(x, fCurYBase + fTopCY + fNearCY - 2.0 + kbBaseY), vec2(w, 2.0),
                         darker, darker, darker, darker); return;
            }
            if (kind == 3 && j == 60) {
                float fMXGap = floor(float(keyWidthAt(j, WinW)) * 0.25 + 0.5);
                float fMCX = float(keyWidthAt(j, WinW)) - fMXGap * 2.0 - fKeyGap;
                float fMY = max(fCurYBase + fTopCY + fNearCY - fMCX - 7.0, fCurYBase + fSharpCY + 5.0);
                uint dc = 0xFF000000u | r2 | (g2 << 8u) | (b2 << 16u);
                emitRect(vid, vec2(fCurX + fKeyGap1 + fMXGap, fMY + kbBaseY),
                         vec2(fMCX, fCurYBase + fTopCY + fNearCY - 7.0 - fMY),
                         dc, dc, dc, dc); return;
            }
        }

        if (kind == 4) {
            float ex = floor(fCurX + fKeyGap1 + float(keyWidthAt(j, WinW)) - fKeyGap + 0.5);
            emitRect(vid, vec2(ex, fCurYBase + kbBaseY), vec2(fKeyGap, fTopCY + fNearCY),
                     0xFF000000u, 0xFF999999u, 0xFF999999u, 0xFF000000u); return;
        }

        emitDegenerate();
        return;
    }

    // Black keys: 53 keys * 6 prims = 318, starting after white
    int blackBase = whiteBase + whitePrimCount;
    int blackPrimCount = 53 * 6;
    if (prim < blackBase + blackPrimCount) {
        int bp = prim - blackBase;
        int i = bp / 6;       // 0..52 over KeyMap[75..127]
        int kind = bp % 6;
        int j = KeyMapArr[75 + i];

        int n = j % 12;
        float fNudgeX = 0.2;
        if (n==1||n==6) fNudgeX = 0.203;
        else if (n==3||n==10) fNudgeX = 0.297;
        else if (n==8) fNudgeX = 0.278;

        float fCurX = float(keyXAt(j, WinW));
        float cx = kw0 * SharpRatio;
        float x = fCurX - kw0 * (SharpRatio / 2.0 - fNudgeX);
        float fSharpTop = SharpRatio * 0.7;
        float fSharpTopX1 = x + kw0 * (SharpRatio - fSharpTop) / 2.0;
        float fSharpTopX2 = fSharpTopX1 + kw0 * fSharpTop;
        float fCurY = fTransitionCY + fRedCY + fSpacerCY;

        bool pressed = keyState[j].pressed != 0u;
        if (!pressed) {
            if (kind == 0) emitSkew(vid,
                vec2(fSharpTopX1, fCurY + fSharpCY - fNearCY + kbBaseY), vec2(fSharpTopX2, fCurY + fSharpCY - fNearCY + kbBaseY),
                vec2(x + cx, fCurY + fSharpCY + kbBaseY), vec2(x, fCurY + fSharpCY + kbBaseY),
                0xFF404040u,0xFF404040u,0xFF000000u,0xFF000000u);
            else if (kind == 1) emitSkew(vid,
                vec2(fSharpTopX1, fCurY - fNearCY + kbBaseY), vec2(fSharpTopX1, fCurY + fSharpCY - fNearCY + kbBaseY),
                vec2(x, fCurY + fSharpCY + kbBaseY), vec2(x, fCurY + kbBaseY),
                0xFF404040u,0xFF404040u,0xFF000000u,0xFF000000u);
            else if (kind == 2) emitSkew(vid,
                vec2(fSharpTopX2, fCurY + fSharpCY - fNearCY + kbBaseY), vec2(fSharpTopX2, fCurY - fNearCY + kbBaseY),
                vec2(x + cx, fCurY + kbBaseY), vec2(x + cx, fCurY + fSharpCY + kbBaseY),
                0xFF404040u,0xFF404040u,0xFF000000u,0xFF000000u);
            else if (kind == 3) emitRect(vid,
                vec2(fSharpTopX1, fCurY - fNearCY + kbBaseY), vec2(fSharpTopX2 - fSharpTopX1, fSharpCY),
                0xFF000000u,0xFF000000u,0xFF000000u,0xFF000000u);
            else if (kind == 4) emitSkew(vid,
                vec2(fSharpTopX1, fCurY - fNearCY + kbBaseY), vec2(fSharpTopX2, fCurY - fNearCY + kbBaseY),
                vec2(fSharpTopX2, fCurY - fNearCY + fSharpCY * 0.45 + kbBaseY), vec2(fSharpTopX1, fCurY - fNearCY + fSharpCY * 0.35 + kbBaseY),
                0xFF202020u,0xFF202020u,0xFF404040u,0xFF404040u);
            else emitSkew(vid,
                vec2(fSharpTopX1, fCurY - fNearCY + fSharpCY * 0.35 + kbBaseY), vec2(fSharpTopX2, fCurY - fNearCY + fSharpCY * 0.45 + kbBaseY),
                vec2(fSharpTopX2, fCurY - fNearCY + fSharpCY * 0.65 + kbBaseY), vec2(fSharpTopX1, fCurY - fNearCY + fSharpCY * 0.55 + kbBaseY),
                0xFF404040u,0xFF404040u,0xFF000000u,0xFF000000u);
            return;
        } else {
            float fNewNear = fNearCY * 0.25;
            uint c = keyState[j].color;
            uint r = (c >> 16u) & 0xFFu;
            uint g = (c >> 8u) & 0xFFu;
            uint b = c & 0xFFu;
            uint r1 = uint(float(r) * 0.5);
            uint g1 = uint(float(g) * 0.5);
            uint b1 = uint(float(b) * 0.5);
            uint c_bgr = 0xFF000000u | (b << 16u) | (g << 8u) | r;
            uint darker = 0xFF000000u | (b1 << 16u) | (g1 << 8u) | r1;

            if (kind == 0) emitSkew(vid,
                vec2(fSharpTopX1, fCurY + fSharpCY - fNewNear + kbBaseY), vec2(fSharpTopX2, fCurY + fSharpCY - fNewNear + kbBaseY),
                vec2(x + cx, fCurY + fSharpCY + kbBaseY), vec2(x, fCurY + fSharpCY + kbBaseY),
                c_bgr,c_bgr,darker,darker);
            else if (kind == 1) emitSkew(vid,
                vec2(fSharpTopX1, fCurY - fNewNear + kbBaseY), vec2(fSharpTopX1, fCurY + fSharpCY - fNewNear + kbBaseY),
                vec2(x, fCurY + fSharpCY + kbBaseY), vec2(x, fCurY + kbBaseY),
                c_bgr,c_bgr,darker,darker);
            else if (kind == 2) emitSkew(vid,
                vec2(fSharpTopX2, fCurY + fSharpCY - fNewNear + kbBaseY), vec2(fSharpTopX2, fCurY - fNewNear + kbBaseY),
                vec2(x + cx, fCurY + kbBaseY), vec2(x + cx, fCurY + fSharpCY + kbBaseY),
                c_bgr,c_bgr,darker,darker);
            else if (kind == 3) emitRect(vid,
                vec2(fSharpTopX1, fCurY - fNewNear + kbBaseY), vec2(fSharpTopX2 - fSharpTopX1, fSharpCY),
                darker,darker,darker,darker);
            else if (kind == 4) emitSkew(vid,
                vec2(fSharpTopX1, fCurY - fNewNear + kbBaseY), vec2(fSharpTopX2, fCurY - fNewNear + kbBaseY),
                vec2(fSharpTopX2, fCurY - fNewNear + fSharpCY * 0.35 + kbBaseY), vec2(fSharpTopX1, fCurY - fNewNear + fSharpCY * 0.25 + kbBaseY),
                c_bgr,c_bgr,c_bgr,c_bgr);
            else emitSkew(vid,
                vec2(fSharpTopX1, fCurY - fNewNear + fSharpCY * 0.25 + kbBaseY), vec2(fSharpTopX2, fCurY - fNewNear + fSharpCY * 0.35 + kbBaseY),
                vec2(fSharpTopX2, fCurY - fNewNear + fSharpCY * 0.75 + kbBaseY), vec2(fSharpTopX1, fCurY - fNewNear + fSharpCY * 0.65 + kbBaseY),
                c_bgr,c_bgr,darker,darker);
            return;
        }
    }

    emitDegenerate();
}