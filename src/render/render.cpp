#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <unordered_map>

#include <backend_render/imgui_impl_sdl3.h>
#include <backend_render/imgui_impl_sdlrenderer3.h>
#include <imgui.h>

#include "../globals.h"
#include "../logger.h"
#include "note_buffer.h"
#include "render.h"














int                _KeyWidth[128];
const f32          SharpRatio = 0.65f;
f32                fDeflate;

// Define the vertex structure
SDL_Vertex         vert[4];

// clang-format off
const us_char Render::KeyMap[128] =
{
    0,   2,   4,   5,   7,   9,   11,  12,  14,  16,  17,  19,  21,  23,  24,  26,
    28,  29,  31,  33,  35,  36,  38,  40,  41,  43,  45,  47,  48,  50,  52,  53,
    55,  57,  59,  60,  62,  64,  65,  67,  69,  71,  72,  74,  76,  77,  79,  81,
    83,  84,  86,  88,  89,  91,  93,  95,  96,  98,  100, 101, 103, 105, 107, 108,
    110, 112, 113, 115, 117, 119, 120, 122, 124, 125, 127, 1,   3,   6,   8,   10,
    13,  15,  18,  20,  22,  25,  27,  30,  32,  34,  37,  39,  42,  44,  46,  49,
    51,  54,  56,  58,  61,  63,  66,  68,  70,  73,  75,  78,  80,  82,  85,  87,
    90,  92,  94,  97,  99,  102, 104, 106, 109, 111, 114, 116, 118, 121, 123, 126,
};
// clang-format on
us_int             Render::note_color;

static const short GenKeyX[] = { 0, 12, 18, 33, 36, 54, 66, 72, 85, 90, 105, 108 };

const char *video_driver;


bool Render::isDesktopSession()
{
    return strcmp(video_driver, "wayland") == 0 || strcmp(video_driver, "x11") == 0;
}

/*
    ▗▄▄▖ ▗▄▖ ▗▖  ▗▖ ▗▄▄▖▗▄▄▄▖▗▄▄▖ ▗▖ ▗▖ ▗▄▄▖▗▄▄▄▖▗▄▖ ▗▄▄▖
   ▐▌   ▐▌ ▐▌▐▛▚▖▐▌▐▌     █  ▐▌ ▐▌▐▌ ▐▌▐▌     █ ▐▌ ▐▌▐▌ ▐▌
   ▐▌   ▐▌ ▐▌▐▌ ▝▜▌ ▝▀▚▖  █  ▐▛▀▚▖▐▌ ▐▌▐▌     █ ▐▌ ▐▌▐▛▀▚▖
   ▝▚▄▄▖▝▚▄▞▘▐▌  ▐▌▗▄▄▞▘  █  ▐▌ ▐▌▝▚▄▞▘▝▚▄▄▖  █ ▝▚▄▞▘▐▌ ▐▌




   This is where the window creation takes place
*/

Render::Render()
{
    SDL_Init(SDL_INIT_VIDEO);
    // IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG); // Things are more simple than last
    // time

    video_driver = SDL_GetCurrentVideoDriver();

    Log::info("", "SDL Version: %d", SDL_GetVersion());

#ifdef PLATFORM_ANDROID
    Win = SDL_CreateWindow("PFA Android", 1920, 1080, 0);
#else
    Win = SDL_CreateWindow("PFA SDL", 1912, 1000, SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
    SDL_SetWindowMinimumSize(Win, 745, 716);
#endif

    if(Win == nullptr)
    {
        Log::trace("The fukin window is not windowing bruh");
        if(isDesktopSession())
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!!!!!", "Failed to create window", nullptr);
    }

#ifdef PLATFORM_ANDROID
    Ren = SDL_CreateRenderer(Win, "opengles2");
#else
    Ren = SDL_CreateRenderer(Win, "gpu");
    if(Ren == nullptr)
    {
        Log::trace("", "Your GPU isn't cooperating with us today. Let's get revenge on it!!! | SDL_GetError(): %s", SDL_GetError());
        if(isDesktopSession())
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!!!!!", "Failed to create render context", nullptr);
        
        Ren = SDL_CreateRenderer(Win, "opengles2"); // This works well on DRM from tty sessions
    }
#endif

    // const char *backend = SDL_GetRendererName(Ren);
    // Log::info("", "SDL_GetRendererName(): %s", backend);

    // SDL_SetRenderVSync(Ren, live_conf.vsync);
    if(!SDL_SetRenderVSync(Ren, live_conf.vsync))
        Log::error("Failed to set vsync: %s", SDL_GetError());

    // Don't set window position on Wayland as it can throw errors on exit
#ifndef PLATFORM_ANDROID
    if(strcmp(video_driver, "wayland") != 0)
        SDL_SetWindowPosition(Win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
#else
    SDL_SetWindowPosition(Win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
#endif
    SDL_DisplayID sid = SDL_GetDisplayForWindow(Win);
    mod               = SDL_GetDesktopDisplayMode(sid);
    SDL_SetRenderDrawBlendMode(Ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColorFloat(Ren, 0, 0, 0, 0);
    SDL_GetWindowSize(Win, &WinW, &WinH);

    // Set note edges at startup (idk much more)
    TW = scale(684), TH = scale(610);
    BkeyW = scale(60), WkeyW = scale(94);
    BkeyH = scale(386), WkeyH = scale(608);
    fDeflate = WkeyW * 0.15f / 2.0f;

    HandleResize(WinW, WinH);
}

/*
    ▗▄▄▄ ▗▄▄▄▖ ▗▄▄▖▗▄▄▄▖▗▄▄▖ ▗▖ ▗▖ ▗▄▄▖▗▄▄▄▖▗▄▖ ▗▄▄▖
    ▐▌  █▐▌   ▐▌     █  ▐▌ ▐▌▐▌ ▐▌▐▌     █ ▐▌ ▐▌▐▌ ▐▌
    ▐▌  █▐▛▀▀▘ ▝▀▚▖  █  ▐▛▀▚▖▐▌ ▐▌▐▌     █ ▐▌ ▐▌▐▛▀▚▖
    ▐▙▄▄▀▐▙▄▄▖▗▄▄▞▘  █  ▐▌ ▐▌▝▚▄▞▘▝▚▄▄▖  █ ▝▚▄▞▘▐▌ ▐▌




*/
Render::~Render()
{
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyTexture(background_img);
    SDL_DestroyRenderer(Ren);
    SDL_DestroyWindow(Win);
    SDL_Quit();
}

/*
    ▗▄▄▖ ▗▄▄▄▖ ▗▄▄▖▗▄▄▄▖▗▄▄▄▄▖▗▄▄▄▖
    ▐▌ ▐▌▐▌   ▐▌     █     ▗▞▘▐▌
    ▐▛▀▚▖▐▛▀▀▘ ▝▀▚▖  █   ▗▞▘  ▐▛▀▀▘
    ▐▌ ▐▌▐▙▄▄▖▗▄▄▞▘▗▄█▄▖▐▙▄▄▄▖▐▙▄▄▖



    ▗▖ ▗▖ ▗▄▖ ▗▖  ▗▖▗▄▄▄ ▗▖   ▗▄▄▄▖▗▖  ▗▖ ▗▄▄▖
    ▐▌ ▐▌▐▌ ▐▌▐▛▚▖▐▌▐▌  █▐▌     █  ▐▛▚▖▐▌▐▌
    ▐▛▀▜▌▐▛▀▜▌▐▌ ▝▜▌▐▌  █▐▌     █  ▐▌ ▝▜▌▐▌▝▜▌
    ▐▌ ▐▌▐▌ ▐▌▐▌  ▐▌▐▙▄▄▀▐▙▄▄▖▗▄█▄▖▐▌  ▐▌▝▚▄▞▘




    Necessary for notes, keyboard and background grid (Vertical lines, See
   line: 643)
*/
void Render::HandleResize(int newWidth, int newHeight)
{
    WinW = newWidth;
    WinH = newHeight;

    // Keyboard resizing + re-position
    for(int i = 0; i != 128; i++)
        KeyX[i] = (i / 12 * 126 + GenKeyX[i % 12]) * WinW / 1350;

    for(int i = 0; i != 127; i++)
    {
        int val;
        switch(i % 12)
        {
        case 1:
        case 3:
        case 6:
        case 8:
        case 10:
            val = WinW * 9 / 1350;
            break;
        case 4:
        case 11:
            val = KeyX[i + 1] - KeyX[i];
            break;
        default:
            val = KeyX[i + 2] - KeyX[i];
            break;
        }
        _KeyWidth[i] = val;
    }
    _KeyWidth[127] = WinW - KeyX[127];

    // Re-scale the notes too
    BkeyW          = scale(60);
    WkeyW          = scale(94);
}

int Render::scale(int x)
{
    // Fixed a small note glitch LMfaoerGdfgg
    return (x * WinW + 4700) / 7330;
}

/*
    ▗▄▄▄ ▗▄▄▖  ▗▄▖ ▗▖ ▗▖▗▄▄▄▖▗▖  ▗▖ ▗▄▄▖
    ▐▌  █▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌  █  ▐▛▚▖▐▌▐▌
    ▐▌  █▐▛▀▚▖▐▛▀▜▌▐▌ ▐▌  █  ▐▌ ▝▜▌▐▌▝▜▌
    ▐▙▄▄▀▐▌ ▐▌▐▌ ▐▌▐▙█▟▌▗▄█▄▖▐▌  ▐▌▝▚▄▞▘



    ▗▄▄▄▖▗▖ ▗▖▗▖  ▗▖ ▗▄▄▖▗▄▄▄▖▗▄▄▄▖ ▗▄▖ ▗▖  ▗▖ ▗▄▄▖
    ▐▌   ▐▌ ▐▌▐▛▚▖▐▌▐▌     █    █  ▐▌ ▐▌▐▛▚▖▐▌▐▌
    ▐▛▀▀▘▐▌ ▐▌▐▌ ▝▜▌▐▌     █    █  ▐▌ ▐▌▐▌ ▝▜▌ ▝▀▚▖
    ▐▌   ▝▚▄▞▘▐▌  ▐▌▝▚▄▄▖  █  ▗▄█▄▖▝▚▄▞▘▐▌  ▐▌▗▄▄▞▘



    Color order: A B R G
*/

void DrawRect(SDL_Renderer *renderer, f32 x, f32 y, f32 cx, f32 cy, u32 c1, u32 c2, u32 c3, u32 c4)
{
    auto getAlpha = [](u32 c)
    {
        return ((c & 0xFF000000) >> 24) / 255.0f;
    };

    // Top
    vert[0].position.x = x;
    vert[0].position.y = y;
    vert[0].color.r    = (c1 & 0xFF) / 255.0f;
    vert[0].color.g    = ((c1 & 0xFF00) >> 8) / 255.0f;
    vert[0].color.b    = ((c1 & 0xFF0000) >> 16) / 255.0f;
    vert[0].color.a    = getAlpha(c1);

    // left
    vert[1].position.x = x + cx;
    vert[1].position.y = y;
    vert[1].color.r    = (c2 & 0xFF) / 255.0f;
    vert[1].color.g    = ((c2 & 0xFF00) >> 8) / 255.0f;
    vert[1].color.b    = ((c2 & 0xFF0000) >> 16) / 255.0f;
    vert[1].color.a    = getAlpha(c2);

    // right
    vert[2].position.x = x + cx;
    vert[2].position.y = y + cy;
    vert[2].color.r    = (c3 & 0xFF) / 255.0f;
    vert[2].color.g    = ((c3 & 0xFF00) >> 8) / 255.0f;
    vert[2].color.b    = ((c3 & 0xFF0000) >> 16) / 255.0f;
    vert[2].color.a    = getAlpha(c3);

    // Bottom
    vert[3].position.x = x;
    vert[3].position.y = y + cy;
    vert[3].color.r    = (c4 & 0xFF) / 255.0f;
    vert[3].color.g    = ((c4 & 0xFF00) >> 8) / 255.0f;
    vert[3].color.b    = ((c4 & 0xFF0000) >> 16) / 255.0f;
    vert[3].color.a    = getAlpha(c4);
    int indices[]      = { 0, 1, 2, 2, 3, 0 };

    // Call SDL_RenderGeometry to draw the quadrilateral.
    SDL_RenderGeometry(renderer, NULL, vert, 4, indices, 6);
}

void DrawSkew(SDL_Renderer *renderer, f32 x1, f32 y1, f32 x2, f32 y2, f32 x3, f32 y3, f32 x4, f32 y4, u32 c1, u32 c2, u32 c3, u32 c4)
{
    auto getAlpha = [](u32 c)
    {
        f32 a = ((c & 0xFF000000) >> 24) / 255.0f;
        return a > 0 ? a : 1.0f;
    };

    // Top
    vert[0].position.x = x1;
    vert[0].position.y = y1;
    vert[0].color.r    = (c1 & 0xFF) / 255.0f;
    vert[0].color.g    = ((c1 & 0xFF00) >> 8) / 255.0f;
    vert[0].color.b    = ((c1 & 0xFF0000) >> 16) / 255.0f;
    vert[0].color.a    = getAlpha(c1);

    // left
    vert[1].position.x = x2;
    vert[1].position.y = y2;
    vert[1].color.r    = (c2 & 0xFF) / 255.0f;
    vert[1].color.g    = ((c2 & 0xFF00) >> 8) / 255.0f;
    vert[1].color.b    = ((c2 & 0xFF0000) >> 16) / 255.0f;
    vert[1].color.a    = getAlpha(c2);

    // right
    vert[2].position.x = x3;
    vert[2].position.y = y3;
    vert[2].color.r    = (c3 & 0xFF) / 255.0f;
    vert[2].color.g    = ((c3 & 0xFF00) >> 8) / 255.0f;
    vert[2].color.b    = ((c3 & 0xFF0000) >> 16) / 255.0f;
    vert[2].color.a    = getAlpha(c3);

    // Bottom
    vert[3].position.x = x4;
    vert[3].position.y = y4;
    vert[3].color.r    = (c4 & 0xFF) / 255.0f;
    vert[3].color.g    = ((c4 & 0xFF00) >> 8) / 255.0f;
    vert[3].color.b    = ((c4 & 0xFF0000) >> 16) / 255.0f;
    vert[3].color.a    = getAlpha(c4);
    int indices[]      = { 0, 1, 2, 2, 3, 0 };

    // Call SDL_RenderGeometry to draw the quadrilateral.
    SDL_RenderGeometry(renderer, NULL, vert, 4, indices, 6);
}

/*
    ▗▖ ▗▖▗▄▄▄▖▗▖  ▗▖▗▄▄▖  ▗▄▖  ▗▄▖ ▗▄▄▖ ▗▄▄▄
    ▐▌▗▞▘▐▌    ▝▚▞▘ ▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌▐▌  █
    ▐▛▚▖ ▐▛▀▀▘  ▐▌  ▐▛▀▚▖▐▌ ▐▌▐▛▀▜▌▐▛▀▚▖▐▌  █
    ▐▌ ▐▌▐▙▄▄▖  ▐▌  ▐▙▄▞▘▝▚▄▞▘▐▌ ▐▌▐▌ ▐▌▐▙▄▄▀



    ▗▄▄▄ ▗▄▄▖  ▗▄▖ ▗▖ ▗▖▗▄▄▄▖▗▖  ▗▖ ▗▄▄▖
    ▐▌  █▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌  █  ▐▛▚▖▐▌▐▌
    ▐▌  █▐▛▀▚▖▐▛▀▜▌▐▌ ▐▌  █  ▐▌ ▝▜▌▐▌▝▜▌
    ▐▙▄▄▀▐▌ ▐▌▐▌ ▐▌▐▙█▟▌▗▄█▄▖▐▌  ▐▌▝▚▄▞▘



*/

void Render::DrawKeyBoard()
{
    f32 fCurX          = 0;
    f32 fTransitionPct = .02f;
    f32 fRedPct        = .05f;
    f32 fSpacerCY      = 2.0f;
    f32 fRedCY         = floor(WinW * 82.0 / 1000 * fRedPct + 0.5f);
    f32 fTransitionCY  = std::max(3.0f, std::floor((f32)WinW * 82 / 1000 * fTransitionPct + 0.5f));
    f32 fTopCY         = floor((WinW * 82.0 / 1000 - fSpacerCY - fRedCY - fTransitionCY) * 0.95f + 0.5f);
    f32 fNearCY        = WinW * 82.0 / 1000 - fSpacerCY - fRedCY - fTransitionCY - fTopCY;
    f32 fKeyGap        = std::max(1.0f, std::floor(_KeyWidth[0] * 0.05f + 0.5f));
    f32 fKeyGap1       = fKeyGap - floor(fKeyGap / 2.0f + 0.5f);
    f32 fCurY          = fTransitionCY + fRedCY + fSpacerCY;
    f32 fSharpCY       = fTopCY * 0.67f;

    // Top Gradient
    f32 fTopGradCY     = (WinW * 82.0 / 1000 - fTransitionCY - fRedCY - fSpacerCY) * 0.02f;
    DrawRect(Ren, 0, WinH - WinW * 82.0 / 1000 + fTransitionCY - fTopGradCY, WinW, fTopGradCY, 0x00333333, 0x00333333, 0xFF000000, 0xFF000000);

    // Red gradient bar over the black bar
    DrawRect(Ren, 0, WinH - WinW * 82.0 / 1000 + fTransitionCY, WinW, fRedCY, 0xFF06054C, 0xFF06054C, 0xFF0D0A98, 0xFF0D0A98);

    // Black spacer between red bar and keys #3D3D3D
    DrawRect(Ren, 0, WinH - WinW * 82.0 / 1000 + fTransitionCY + fRedCY, WinW, fSpacerCY, 0xFF3D3D3D, 0xFF3D3D3D, 0xFF3D3D3D, 0xFF3D3D3D);

    for(int i = 0; i != 75; i++)
    {
        int j = KeyMap[i];
        if(!KeyPress[j]) // If the key is not pressed
        {
            // Key body
            DrawRect(Ren, fCurX + fKeyGap1, fCurY + WinH - WinW * 82.0 / 1000, _KeyWidth[j] - fKeyGap, fTopCY + fNearCY, 0xFFCCCCCC, 0xFFCCCCCC, 0xFFFFFFFF, 0xFFFFFFFF);

            // The bottom side of the key with a subtle gradient
            DrawRect(Ren, fCurX + fKeyGap1, fCurY + fTopCY + WinH - WinW * 82.0 / 1000, _KeyWidth[j] - fKeyGap, 2.4f, 0xFF242424, 0xFF242424, 0xFFA3A3A3, 0xFFA3A3A3);
            DrawRect(Ren, fCurX + fKeyGap1, fCurY + fTopCY + WinH - WinW * 80.5 / 1000, _KeyWidth[j] - fKeyGap, 6.0f, 0xFFBABABA, 0xFFBABABA, 0xFF9C9C9C, 0xFF9C9C9C);

            // Middle C square on C60
            if(j == 60)
            {
                f32 fMXGap = floor(_KeyWidth[j] * 0.25f + 0.5f);
                f32 fMCX   = _KeyWidth[j] - fMXGap * 2.0f - fKeyGap;
                f32 fMY    = std::max(fCurY + fTopCY - fMCX - 5.0f, fCurY + fSharpCY + 5.0f);
                DrawRect(Ren, fCurX + fKeyGap1 + fMXGap, fMY + WinH - WinW * 82.0 / 1000, fMCX, fCurY + fTopCY - 5.0f - fMY, 0xFFCCCCCC, 0xFFCCCCCC, 0xFFCCCCCC, 0xFFCCCCCC);
            }
        }
        else
        {
            us_int   c      = KeyColor[j];
            us_short r      = (c >> 16) & 0xFF;
            us_short g      = (c >> 8) & 0xFF;
            us_short b      = c & 0xFF;
            us_short r2     = r * 0.6f;
            us_short g2     = g * 0.6f;
            us_short b2     = b * 0.6f;
            us_int   c_bgr  = 0xFF000000 | (b << 16) | (g << 8) | r;
            us_int   darker = 0xFF000000 | (b2 << 16) | (g2 << 8) | r2;

            // Draw the colored fill of the white keys
            DrawRect(Ren, fCurX + fKeyGap1, fCurY + WinH - WinW * 82.0 / 1000, _KeyWidth[j] - fKeyGap, fTopCY + fNearCY - 2.0f, darker, darker, c_bgr, c_bgr);

            // Draw the colored bottom of the white key when pressed
            DrawRect(Ren, fCurX + fKeyGap1, fCurY + fTopCY + fNearCY - 2.0f + WinH - WinW * 82.0 / 1000, _KeyWidth[j] - fKeyGap, 2.0f, darker, darker, darker, darker);

            // Middle C square on C60 key but darker
            if(j == 60)
            {
                f32 fMXGap = floor(_KeyWidth[j] * 0.25f + 0.5f);
                f32 fMCX   = _KeyWidth[j] - fMXGap * 2.0f - fKeyGap;
                f32 fMY    = std::max(fCurY + fTopCY + fNearCY - fMCX - 7.0f, fCurY + fSharpCY + 5.0f);
                // DrawRect(Ren, fCurX + fKeyGap1+WinH - WinW * 82 / 1000 +
                // fMXGap, fMY, fMCX, fCurY + fTopCY + fNearCY - 7.0f - fMY,
                // m_csKBWhite.iDarkRGB );
                DrawRect(Ren, fCurX + fKeyGap1 + fMXGap, fMY + WinH - WinW * 82.0 / 1000, fMCX, fCurY + fTopCY + fNearCY - 7.0f - fMY, 0xFF000000 | r2 | g2 << 8 | b2 << 16, 0xFF000000 | r2 | g2 << 8 | b2 << 16, 0xFF000000 | r2 | g2 << 8 | b2 << 16, 0xFF000000 | r2 | g2 << 8 | b2 << 16);
            }
        }

        // Gray edges of the white keys (No note color mixing is done)
        DrawRect(Ren, floor(fCurX + fKeyGap1 + _KeyWidth[j] - fKeyGap + 0.5f), fCurY + WinH - WinW * 82.0 / 1000, fKeyGap, fTopCY + fNearCY, 0xFF000000, 0xFF999999, 0xFF999999, 0xFF000000);
        fCurX += _KeyWidth[j];
    }

    f32 fSharpTop = SharpRatio * 0.7f;

    fCurY         = fTransitionCY + fRedCY + fSpacerCY;

    for(int i = 75; i != 128; i++)
    {
        int j                 = KeyMap[i];
        f32 fNudgeX           = 0.2f;
        {
            int n = j % 12;
            if(n == 1 || n == 6)      fNudgeX =  0.203f;  // C#, F# — match PFA center
            else if(n == 3 || n == 10) fNudgeX =  0.297f;  // D#, A#
            else if(n == 8)            fNudgeX =  0.278f;  // G#
        }
        fCurX                 = KeyX[j];
        const f32 cx          = _KeyWidth[0] * SharpRatio;
        const f32 x           = fCurX - _KeyWidth[0] * (SharpRatio / 2.0f - fNudgeX);
        const f32 fSharpTopX1 = x + _KeyWidth[0] * (SharpRatio - fSharpTop) / 2.0f;
        const f32 fSharpTopX2 = fSharpTopX1 + _KeyWidth[0] * fSharpTop;

        if(!KeyPress[j]) // If the key is not pressed
        {
            // Black keys bottom end
            DrawSkew(Ren, fSharpTopX1, fCurY + fSharpCY - fNearCY + WinH - WinW * 82.0 / 1000, fSharpTopX2, fCurY + fSharpCY - fNearCY + WinH - WinW * 82.0 / 1000, x + cx, fCurY + fSharpCY + WinH - WinW * 82.0 / 1000, x, fCurY + fSharpCY + WinH - WinW * 82.0 / 1000, 0xFF404040, 0xFF404040, 0xFF000000, 0xFF000000);

            // Left side of the black keys
            DrawSkew(Ren, fSharpTopX1, fCurY - fNearCY + WinH - WinW * 82.0 / 1000, fSharpTopX1, fCurY + fSharpCY - fNearCY + WinH - WinW * 82.0 / 1000, x, fCurY + fSharpCY + WinH - WinW * 82.0 / 1000, x, fCurY + WinH - WinW * 82.0 / 1000, 0xFF404040, 0xFF404040, 0xFF000000, 0xFF000000);

            // Right side of the black keys
            DrawSkew(Ren, fSharpTopX2, fCurY + fSharpCY - fNearCY + WinH - WinW * 82.0 / 1000, fSharpTopX2, fCurY - fNearCY + WinH - WinW * 82.0 / 1000, x + cx, fCurY + WinH - WinW * 82.0 / 1000, x + cx, fCurY + fSharpCY + WinH - WinW * 82.0 / 1000, 0xFF404040, 0xFF404040, 0xFF000000, 0xFF000000);

            // Bottom half gradient of the black keys
            DrawRect(Ren, fSharpTopX1, fCurY - fNearCY + WinH - WinW * 82.0 / 1000, fSharpTopX2 - fSharpTopX1, fSharpCY, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000);

            // Top half gradient of the black keys
            DrawSkew(Ren, fSharpTopX1, fCurY - fNearCY + WinH - WinW * 82.0 / 1000, fSharpTopX2, fCurY - fNearCY + WinH - WinW * 82.0 / 1000, fSharpTopX2, fCurY - fNearCY + fSharpCY * 0.45f + WinH - WinW * 82.0 / 1000, fSharpTopX1, fCurY - fNearCY + fSharpCY * 0.35f + WinH - WinW * 82.0 / 1000, 0xFF202020, 0xFF202020, 0xFF404040, 0xFF404040);

            // Middle gradient (?)
            DrawSkew(Ren, fSharpTopX1, fCurY - fNearCY + fSharpCY * 0.35f + WinH - WinW * 82.0 / 1000, fSharpTopX2, fCurY - fNearCY + fSharpCY * 0.45f + WinH - WinW * 82.0 / 1000, fSharpTopX2, fCurY - fNearCY + fSharpCY * 0.65f + WinH - WinW * 82.0 / 1000, fSharpTopX1, fCurY - fNearCY + fSharpCY * 0.55f + WinH - WinW * 82.0 / 1000, 0xFF404040, 0xFF404040, 0xFF000000, 0xFF000000);
        }
        else
        {
            const f32 fNewNear = fNearCY * 0.25f;
            us_int    c        = KeyColor[j];
            us_short  r        = (c >> 16) & 0xFF;
            us_short  g        = (c >> 8) & 0xFF;
            us_short  b        = c & 0xFF;
            us_short  r1       = r * 0.5f;
            us_short  g1       = g * 0.5f;
            us_short  b1       = b * 0.5f;
            us_int    c_bgr    = 0xFF000000 | (b << 16) | (g << 8) | r;
            us_int    darker   = 0xFF000000 | (b1 << 16) | (g1 << 8) | r1;

            // Black keys bottom end
            DrawSkew(Ren, fSharpTopX1, fCurY + fSharpCY - fNewNear + WinH - WinW * 82.0 / 1000, fSharpTopX2, fCurY + fSharpCY - fNewNear + WinH - WinW * 82.0 / 1000, x + cx, fCurY + fSharpCY + WinH - WinW * 82.0 / 1000, x, fCurY + fSharpCY + WinH - WinW * 82.0 / 1000, c_bgr, c_bgr, darker, darker);

            // Left side of the black keys
            DrawSkew(Ren, fSharpTopX1, fCurY - fNewNear + WinH - WinW * 82.0 / 1000, fSharpTopX1, fCurY + fSharpCY - fNewNear + WinH - WinW * 82.0 / 1000, x, fCurY + fSharpCY + WinH - WinW * 82.0 / 1000, x, fCurY + WinH - WinW * 82.0 / 1000, c_bgr, c_bgr, darker, darker);

            // Right side of the black keys
            DrawSkew(Ren, fSharpTopX2, fCurY + fSharpCY - fNewNear + WinH - WinW * 82.0 / 1000, fSharpTopX2, fCurY - fNewNear + WinH - WinW * 82.0 / 1000, x + cx, fCurY + WinH - WinW * 82.0 / 1000, x + cx, fCurY + fSharpCY + WinH - WinW * 82.0 / 1000, c_bgr, c_bgr, darker, darker);

            // Bottom half gradient of the black keys
            DrawRect(Ren, fSharpTopX1, fCurY - fNewNear + WinH - WinW * 82.0 / 1000, fSharpTopX2 - fSharpTopX1, fSharpCY, darker, darker, darker, darker);

            // Top half gradient of the black keys
            DrawSkew(Ren, fSharpTopX1, fCurY - fNewNear + WinH - WinW * 82.0 / 1000, fSharpTopX2, fCurY - fNewNear + WinH - WinW * 82.0 / 1000, fSharpTopX2, fCurY - fNewNear + fSharpCY * 0.35f + WinH - WinW * 82.0 / 1000, fSharpTopX1, fCurY - fNewNear + fSharpCY * 0.25f + WinH - WinW * 82.0 / 1000, c_bgr, c_bgr, c_bgr, c_bgr);

            // Middle gradient (?)
            DrawSkew(Ren, fSharpTopX1, fCurY - fNewNear + fSharpCY * 0.25f + WinH - WinW * 82.0 / 1000, fSharpTopX2, fCurY - fNewNear + fSharpCY * 0.35f + WinH - WinW * 82.0 / 1000, fSharpTopX2, fCurY - fNewNear + fSharpCY * 0.75f + WinH - WinW * 82.0 / 1000, fSharpTopX1, fCurY - fNewNear + fSharpCY * 0.65f + WinH - WinW * 82.0 / 1000, c_bgr, c_bgr, darker, darker);
        }
    }
}

/*
    ▗▖  ▗▖ ▗▄▖▗▄▄▄▖▗▄▄▄▖ ▗▄▄▖
    ▐▛▚▖▐▌▐▌ ▐▌ █  ▐▌   ▐▌
    ▐▌ ▝▜▌▐▌ ▐▌ █  ▐▛▀▀▘ ▝▀▚▖
    ▐▌  ▐▌▝▚▄▞▘ █  ▐▙▄▄▖▗▄▄▞▘



    ▗▄▄▄ ▗▄▄▖  ▗▄▖ ▗▖ ▗▖▗▄▄▄▖▗▖  ▗▖ ▗▄▄▖
    ▐▌  █▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌  █  ▐▛▚▖▐▌▐▌
    ▐▌  █▐▛▀▚▖▐▛▀▜▌▐▌ ▐▌  █  ▐▌ ▝▜▌▐▌▝▜▌
    ▐▙▄▄▀▐▌ ▐▌▐▌ ▐▌▐▙█▟▌▗▄█▄▖▐▌  ▐▌▝▚▄▞▘



    And also color generation (for random note color settings)
*/
bool Render::IsSharp(int note)
{
    int n = note % 12;
    return (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
}

void Render::CreateNote(int k, int yb, int ye, us_int c)
{

    int x = KeyX[KeyMap[k]] - 1;
    int w = (k >= 75) ? (int)(_KeyWidth[0] * SharpRatio) : WkeyW + 1;
    if(k >= 75)
    {
        int j = KeyMap[k];
        f32 fNudge = 0.2f;
        int n = j % 12;
        if(n == 1 || n == 6)      fNudge = 0.203f;
        else if(n == 3 || n == 10) fNudge = 0.297f;
        else if(n == 8)            fNudge = 0.278f;
        x -= (int)(_KeyWidth[0] * (SharpRatio / 2.0f - fNudge));
    }

    int      h       = yb - ye;

    us_short r       = (c >> 16) & 0xFF;
    us_short g       = (c >> 8) & 0xFF;
    us_short b       = c & 0xFF;

    us_short r1      = r * 0.6f;
    us_short g1      = g * 0.6f;
    us_short b1      = b * 0.6f;

    us_short r2      = r * 0.2f;
    us_short g2      = g * 0.2f;
    us_short b2      = b * 0.2f;

    // Convert original to BGR as well
    us_int   c_bgr   = 0xFF000000 | (b << 16) | (g << 8) | r;
    us_int   darker  = 0xFF000000 | (b2 << 16) | (g2 << 8) | r2;
    us_int   lighter = 0xFF000000 | (b1 << 16) | (g1 << 8) | r1;

    // Draw a darker rect which serves as the note outline
    DrawRect(Ren, x, ye, w, h, darker, darker, darker, darker);

    // Draw a sligtly smaller rect with color blending gradient on top of the
    // darker rect
    if(h - 2.0f * fDeflate > 0)
        DrawRect(Ren, x + fDeflate, ye + fDeflate, w - 2.0f * fDeflate, h - 2.0f * fDeflate, c_bgr, lighter, lighter, c_bgr);
}

void Render::DrawNote(NVMidi::u16_t k, const NVnote &n, int pps)
{

    std::pair<int, int>                                          trackChannelKey = { n.track, n.chn };
    static std::unordered_map<int, decltype(Render::note_color)> _randomColorCache;

    auto                                                         it = NoteBuffer::trackChannelColorMap.find(trackChannelKey);
    int                                                          colorIndex;

    if(it == NoteBuffer::trackChannelColorMap.end())
    {
        // New track/channel combination - assign next color index
        colorIndex                                        = NoteBuffer::trackChannelColorMap.size();
        NoteBuffer::trackChannelColorMap[trackChannelKey] = colorIndex;
    }
    else
        colorIndex = it->second; // Already assigned - use existing index

    if(!live_conf.is_custom_ch_colors)
    {
        if(live_conf.loop_colors)
        {
            note_color = default_settings.channel_colors[colorIndex % 16]; // Something is not quite right here. It should be
                                                                           // default_settings.channel_colors[colorIndex % 16]
            note_color = live_conf.channel_colors[colorIndex % 16];        // This is what allows live
                                                                           // note color changing
        }
        else
        {
            if(colorIndex < 16)
                note_color = default_settings.channel_colors[colorIndex];
            else
            {
                // Render::note_color = GenerateRandomColor();

                auto [it, inserted] = _randomColorCache.try_emplace(colorIndex);
                if(inserted)
                    it->second = NoteBuffer::GenerateRandomColor();
                Render::note_color = it->second;
            }
        }
    }
    else
    {
        if(live_conf.loop_colors)
            note_color = live_conf.channel_colors[colorIndex % 16];
        else
        {
            if(colorIndex < 16)
                note_color = live_conf.channel_colors[colorIndex];
            else
            {
                // Render::note_color = GenerateRandomColor();

                auto [it, inserted] = _randomColorCache.try_emplace(colorIndex);
                if(inserted)
                    it->second = NoteBuffer::GenerateRandomColor();
                Render::note_color = it->second;
            }
        }
    }

    int key = KeyMap[k];

    int y_0 = std::clamp((int)floor(_WinH - (n.Tstart - Playback::Tplay) * pps + 0.5f), 0, _WinH);
    int y_1 = (n.Tend < Playback::Tplay + Tscr) ? std::clamp((int)floor(_WinH - (n.Tend - Playback::Tplay) * pps + 0.5f), 0, _WinH) : 0;

    if(n.Tstart <= Playback::Tplay && Playback::Tplay < n.Tend)
    {
        RenderWin->KeyPress[key] = true;
        RenderWin->KeyColor[key] = note_color;
    }

    RenderWin->CreateNote(k, y_0, y_1, note_color);
}

/*
    ▗▄▄▖  ▗▄▖  ▗▄▄▖▗▖ ▗▖ ▗▄▄▖▗▄▄▖  ▗▄▖ ▗▖ ▗▖▗▖  ▗▖▗▄▄▄
    ▐▌ ▐▌▐▌ ▐▌▐▌   ▐▌▗▞▘▐▌   ▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌▐▛▚▖▐▌▐▌  █
    ▐▛▀▚▖▐▛▀▜▌▐▌   ▐▛▚▖ ▐▌▝▜▌▐▛▀▚▖▐▌ ▐▌▐▌ ▐▌▐▌ ▝▜▌▐▌  █
    ▐▙▄▞▘▐▌ ▐▌▝▚▄▄▖▐▌ ▐▌▝▚▄▞▘▐▌ ▐▌▝▚▄▞▘▝▚▄▞▘▐▌  ▐▌▐▙▄▄▀



    ▗▄▄▄▖▗▖  ▗▖ ▗▄▖  ▗▄▄▖▗▄▄▄▖
      █  ▐▛▚▞▜▌▐▌ ▐▌▐▌   ▐▌
      █  ▐▌  ▐▌▐▛▀▜▌▐▌▝▜▌▐▛▀▀▘
    ▗▄█▄▖▐▌  ▐▌▐▌ ▐▌▝▚▄▞▘▐▙▄▄▖



*/

void Render::LoadBackgroundImage(std::string file)
{
    background_img = IMG_LoadTexture(Ren, file.c_str());

    if(!background_img)
    {
        Log::error("", "Failed to load background image: %s", SDL_GetError());
        Log::info("Now you hopefully learnt to not use stuff like this "
                  "incorrectly. Did you ??");
        return;
    }
}

/*
    ▗▄▄▖  ▗▄▖  ▗▄▄▖▗▖ ▗▖ ▗▄▄▖▗▄▄▖  ▗▄▖ ▗▖ ▗▖▗▖  ▗▖▗▄▄▄
    ▐▌ ▐▌▐▌ ▐▌▐▌   ▐▌▗▞▘▐▌   ▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌▐▛▚▖▐▌▐▌  █
    ▐▛▀▚▖▐▛▀▜▌▐▌   ▐▛▚▖ ▐▌▝▜▌▐▛▀▚▖▐▌ ▐▌▐▌ ▐▌▐▌ ▝▜▌▐▌  █
    ▐▙▄▞▘▐▌ ▐▌▝▚▄▄▖▐▌ ▐▌▝▚▄▞▘▐▌ ▐▌▝▚▄▞▘▝▚▄▞▘▐▌  ▐▌▐▙▄▄▀



     ▗▄▄▖▗▄▄▖ ▗▄▄▄▖▗▄▄▄
    ▐▌   ▐▌ ▐▌  █  ▐▌  █
    ▐▌▝▜▌▐▛▀▚▖  █  ▐▌  █
    ▝▚▄▞▘▐▌ ▐▌▗▄█▄▖▐▙▄▄▀



*/

static void DeriveGridColors(int bgR, int bgG, int bgB,
                             us_int &outDark, us_int &outVeryDark)
{
    // PianoFromAbove style: dark = 0.7x brightness, verydark = 1.3x (brighter, capped)
    int dR  = (int)(bgR * 0.7f), dG = (int)(bgG * 0.7f), dB = (int)(bgB * 0.7f);
    int vdR = std::min(255, (int)(bgR * 1.3f));
    int vdG = std::min(255, (int)(bgG * 1.3f));
    int vdB = std::min(255, (int)(bgB * 1.3f));
    outDark     = 0xFF000000 | (dB << 16) | (dG << 8) | dR;
    outVeryDark = 0xFF000000 | (vdB << 16) | (vdG << 8) | vdR;
}

void Render::DrawBackgroundGrid()
{
    us_int iDark = 0, iVeryDark = 0;
    DeriveGridColors(live_conf.bg_R, live_conf.bg_G, live_conf.bg_B, iDark, iVeryDark);

    for(int i = 1; i <= 127; i++)
    {
        if(!IsSharp(i - 1) && !IsSharp(i))
        {
            f32 x = KeyX[i - 1] + _KeyWidth[i - 1];
            x     = floorf(x + 0.5f);
            // 3px vertical line with left/right gradient (PianoFromAbove)
            DrawRect(Ren, x - 1.0f, 0.0f, 3.0f, (f32)_WinH,
                iDark, iVeryDark, iVeryDark, iDark);
        }
    }
}

void Render::DrawHorizontalLines()
{
    if(!Playback::is_playback_started) return;

    us_int iDark = 0, iVeryDark = 0;
    DeriveGridColors(live_conf.bg_R, live_conf.bg_G, live_conf.bg_B, iDark, iVeryDark);

    auto &te = Midi_ctx.TempoEvents;
    if(te.empty()) return;

    const int  beatsPerMeasure = 4;
    f64        pps             = (f64)_WinH / Tscr; // pixels per second
    f64        t_cur           = Playback::Tplay;
    f64        t_end           = t_cur + Tscr;

    // Walk tempo segments, drawing a measure line every beatsPerMeasure beats
    f64 beatAccum = 0.0;
    for(size_t s = 0; s < te.size(); s++)
    {
        f64 segStart = te[s].T;
        f64 segEnd   = (s + 1 < te.size()) ? te[s + 1].T : t_end + 1e12;
        if(segEnd <= t_cur)
        {
            beatAccum += (segEnd - segStart) / (te[s].usPerQuarter * 1e-6);
            continue;
        }

        f64 segStartClamped = std::max(segStart, t_cur);
        f64 spb             = te[s].usPerQuarter * 1e-6;
        if(spb <= 0) spb = 0.5;

        f64 relBeat         = beatAccum + (segStartClamped - segStart) / spb;
        f64 nextMeasureBeat = std::ceil(relBeat / beatsPerMeasure) * beatsPerMeasure;
        f64 t               = segStart + (nextMeasureBeat - beatAccum) * spb;

        while(t <= segEnd && t <= t_end)
        {
            f32 y = (f32)_WinH - (f32)((t - t_cur) * pps);
            y = floorf(y + 0.5f);
            if(y >= 0.0f && y <= (f32)_WinH)
                // 3px horizontal line with top/bottom gradient (PianoFromAbove)
                DrawRect(Ren, 0.0f, y - 1.0f, (f32)WinW, 3.0f,
                    iDark, iDark, iVeryDark, iVeryDark);
            nextMeasureBeat += beatsPerMeasure;
            t = segStart + (nextMeasureBeat - beatAccum) * spb;
        }
        beatAccum += (segEnd - segStart) / spb;
    }
}

void Render::clear()
{
    SDL_RenderClear(Ren);
    for(int i = 0; i < 128; i++)
        KeyColor[i] = 0xFFFFFFFF, KeyPress[i] = false;
}
