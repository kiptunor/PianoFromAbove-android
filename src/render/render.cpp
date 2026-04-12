#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cmath>
#include <algorithm>
#include <random>




#include <imgui.h>
#include <backend_render/imgui_impl_sdl3.h>
#include <backend_render/imgui_impl_sdlrenderer3.h>

#include "render.h"
#include "../globals.h"
#include "../logger.h"












int _KeyWidth[128];
int fn_call_index = 0;
const float SharpRatio = 0.64f;
float fDeflate;

// Define the vertex structure
SDL_Vertex vert[4];


const unsigned char KeyMap[128] =
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

static const short GenKeyX[] =
{
    0, 12, 18, 33, 36, 54, 66, 72, 85, 90, 105, 108
};

Render::Render()
{
    SDL_Init(SDL_INIT_VIDEO);
    // IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG); // Things are more simple than last time
    
    #ifdef PLATFORM_ANDROID
        Win = SDL_CreateWindow("PFA Android", 1920, 1080, 0);
    #else
        Win = SDL_CreateWindow("PFA SDL", 1912, 1000, SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
        SDL_SetWindowMinimumSize(Win, 745, 716);
    #endif
    
    if(Win == nullptr)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!!!!!", "Failed to create window" , nullptr);
    }
    
    Ren = SDL_CreateRenderer(Win, "gpu");
    if(Ren == nullptr)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!!!!!", "Failed to create render context" , nullptr);
    }
    
    const char *backend = SDL_GetRendererName(Ren);
    printf("SDL_GetRendererName(): %s\n", backend);
    
    SDL_SetRenderVSync(Ren, live_conf.vsync); // Todo: Add to config and UI settings
	SDL_SetWindowPosition(Win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	
	SDL_DisplayID sid = SDL_GetDisplayForWindow(Win);
	mod = SDL_GetDesktopDisplayMode(sid);
	SDL_SetRenderDrawBlendMode(Ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColorFloat(Ren, 0, 0, 0, 0);
    SDL_GetWindowSize(Win, &WinW, &WinH);
    
    for(int i = 0; i != 128; ++i)
	{
	    KeyX[i] = (i / 12 * 126 + GenKeyX[i % 12]) * WinW / 1350;
	}
   
    for(int i = 0; i != 127; ++i)
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
   
    TW    = scale(684), TH    = scale(610);
    BkeyW = scale(60 ), WkeyW = scale(94 );
    BkeyH = scale(386), WkeyH = scale(608);
	fDeflate = WkeyW * 0.15f / 2.0f;
	fDeflate = floor( fDeflate + 0.5f );
	fDeflate = std::max( std::min( fDeflate, 3.0f ), 1.0f );
}


Render::~Render()
{
    //Log::trace(SRC_STRING.c_str(), "Destructor called");
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyTexture(background_img);
    SDL_DestroyRenderer(Ren);
    SDL_DestroyWindow(Win);
    SDL_Quit();
}

void DrawRect(SDL_Renderer* renderer, float x, float y, float cx, float cy, u32 c1, u32 c2, u32 c3, u32 c4)
{
    // Converts color from DWORD to SDL_Color
    // Alpha: if 0 (no alpha in color), treat as 255 (opaque)
    auto getAlpha = [](u32 c) { float a = ((c&0xFF000000)>>24)/255.0f; return a > 0 ? a : 1.0f; };
    
    vert[0].position.x = x;
    vert[0].position.y = y;
    vert[0].color.r = (c1&0xFF)/255.0f;
    vert[0].color.g = ((c1&0xFF00)>>8)/255.0f;
    vert[0].color.b = ((c1&0xFF0000)>>16)/255.0f;
    vert[0].color.a = getAlpha(c1);

    // left
    vert[1].position.x = x+cx;
    vert[1].position.y = y;
    vert[1].color.r = (c2&0xFF)/255.0f;
    vert[1].color.g = ((c2&0xFF00)>>8)/255.0f;
    vert[1].color.b = ((c2&0xFF0000)>>16)/255.0f;
    vert[1].color.a = getAlpha(c2);

    // right
    vert[2].position.x = x+cx;
    vert[2].position.y = y+cy;
    vert[2].color.r = (c3&0xFF)/255.0f;
    vert[2].color.g = ((c3&0xFF00)>>8)/255.0f;
    vert[2].color.b = ((c3&0xFF0000)>>16)/255.0f;
    vert[2].color.a = getAlpha(c3);
    vert[3].position.x = x;
    vert[3].position.y = y+cy;
    vert[3].color.r = (c4&0xFF)/255.0f;
    vert[3].color.g = ((c4&0xFF00)>>8)/255.0f;
    vert[3].color.b = ((c4&0xFF0000)>>16)/255.0f;
    vert[3].color.a = getAlpha(c4);
    int indices[] = {0, 1, 2, 2, 3, 0};
    // Call SDL_RenderGeometry to draw the quadrilateral.
    SDL_RenderGeometry(renderer, NULL, vert, 4, indices, 6);
}

void DrawSkew(SDL_Renderer* renderer, float x1, float y1, float x2, float y2,float x3, float y3,float x4, float y4, u32 c1, u32 c2, u32 c3, u32 c4)
{
    // Converts color from DWORD to SDL_Color
    auto getAlpha = [](u32 c) { float a = ((c&0xFF000000)>>24)/255.0f; return a > 0 ? a : 1.0f; };
    
    vert[0].position.x = x1;
    vert[0].position.y = y1;
    vert[0].color.r = (c1&0xFF)/255.0f;
    vert[0].color.g = ((c1&0xFF00)>>8)/255.0f;
    vert[0].color.b = ((c1&0xFF0000)>>16)/255.0f;
    vert[0].color.a = getAlpha(c1);

    // left
    vert[1].position.x = x2;
    vert[1].position.y = y2;
    vert[1].color.r = (c2&0xFF)/255.0f;
    vert[1].color.g = ((c2&0xFF00)>>8)/255.0f;
    vert[1].color.b = ((c2&0xFF0000)>>16)/255.0f;
    vert[1].color.a = getAlpha(c2);

    // right
    vert[2].position.x = x3;
    vert[2].position.y = y3;
    vert[2].color.r = (c3&0xFF)/255.0f;
    vert[2].color.g = ((c3&0xFF00)>>8)/255.0f;
    vert[2].color.b = ((c3&0xFF0000)>>16)/255.0f;
    vert[2].color.a = getAlpha(c3);
    vert[3].position.x = x4;
    vert[3].position.y = y4;
    vert[3].color.r = (c4&0xFF)/255.0f;
    vert[3].color.g = ((c4&0xFF00)>>8)/255.0f;
    vert[3].color.b = ((c4&0xFF0000)>>16)/255.0f;
    vert[3].color.a = getAlpha(c4);
    int indices[] = {0, 1, 2, 2, 3, 0};
    // Call SDL_RenderGeometry to draw the quadrilateral.
    SDL_RenderGeometry(renderer, NULL, vert, 4, indices, 6);
}

void Render::clear()
{
    SDL_RenderClear(Ren);

    for(int i = 0; i < 128; ++i)
    {
        KeyColor[i] = 0xFFFFFFFF, KeyPress[i] = false;
    }
}

void Render::CreateNote(int k, int yb, int ye, unsigned int c)
{

    int x  = KeyX[KeyMap[k]]-1, w = (k >= 75? BkeyW : WkeyW)+1;
    int  h = yb - ye;
    
    unsigned short r = (c >> 16) & 0xFF;
    unsigned short g = (c >> 8) & 0xFF;
    unsigned short b = c & 0xFF;
    
    unsigned short r1 = r * 0.6f;
    unsigned short g1 = g * 0.6f;
    unsigned short b1 = b * 0.6f;
    
    unsigned short r2 = r * 0.2f;
    unsigned short g2 = g * 0.2f;
    unsigned short b2 = b * 0.2f;
    
    // Convert original to BGR as well
    unsigned int c_bgr = 0xFF000000 | (b << 16) | (g << 8) | r;
    unsigned int darker = 0xFF000000 | (b2 << 16) | (g2 << 8) | r2;
    unsigned int lighter = 0xFF000000 | (b1 << 16) | (g1 << 8) | r1;
    
    // Draw the inside of the note
    DrawRect(Ren, x, ye, w, h, darker, darker, darker, darker);
    
    // Draw all 4 borders of the note with a darker color
    if(h - 2.0f * fDeflate > 0)
        DrawRect(Ren, x + fDeflate, ye + fDeflate, w - 2.0f * fDeflate, h - 2.0f * fDeflate, c_bgr, lighter, lighter, c_bgr);
}

void Render::DrawKeyBoard()
{
	float fTransitionPct = .02f;
    float fTransitionCY = std::max( 3.0f, std::floor( WinW * 82 / 1000 * fTransitionPct + 0.5f ) );
    float fRedPct = .05f;
    float fRedCY = floor( WinW * 82 / 1000 * fRedPct + 0.5f );
    float fSpacerCY = 2.0f;
    float fTopCY = floor( ( WinW * 82 / 1000 - fSpacerCY - fRedCY - fTransitionCY ) * 0.95f + 0.5f );
    float fNearCY = WinW * 82 / 1000 - fSpacerCY - fRedCY - fTransitionCY - fTopCY;
	float fKeyGap = std::max( 1.0f, std::floor( _KeyWidth[0] * 0.05f + 0.5f ) );
    float fKeyGap1 = fKeyGap - floor( fKeyGap / 2.0f + 0.5f );
	float fCurX = 0;
	float fSharpCY = fTopCY * 0.67f;
    float fCurY = fTransitionCY + fRedCY + fSpacerCY;
    
    // Black bar (with top offset)
	DrawRect(Ren, 0, WinH - WinW * 82 / 1000, WinW, WinW * 82 / 1000, 0xFF000000,0xFF000000,0xFF000000,0xFF000000);
	
	// Red gradient bar over the black bar
    DrawRect(Ren, 0, WinH - WinW * 82 / 1000 + fTransitionCY, WinW, fRedCY, 0xFF06054C, 0xFF06054C, 0xFF0D0A98, 0xFF0D0A98);
    
    // Black spacer between red bar and keys
    DrawRect(Ren, 0, WinH - WinW * 82 / 1000 + fTransitionCY + fRedCY, WinW, fSpacerCY, 0xFF1C1C1C, 0xFF1C1C1C, 0xFF1C1C1C, 0xFF1C1C1C);
    
    
    for(int i = 0; i != 75; ++i)
    {
	    int j = KeyMap[i];
	    if(!KeyPress[j])//If the key is not pressed
        {
	        DrawRect(Ren, fCurX + fKeyGap1 , fCurY+WinH - WinW * 82 / 1000, _KeyWidth[j] - fKeyGap, fTopCY + fNearCY, 0xFFCCCCCC, 0xFFCCCCCC, 0xFFFFFFFF, 0xFFFFFFFF );
            //DrawRect(Ren, fCurX + fKeyGap1 , fCurY + fTopCY+WinH - WinW * 82 / 1000, _KeyWidth[j] - fKeyGap, fNearCY, 0xFFCCCCCC, 0xFFCCCCCC, 0xFF999999, 0xFF999999 );
            // The bottom side of the key
            DrawRect(Ren, fCurX + fKeyGap1, fCurY + fTopCY+WinH - WinW * 82 / 1000, _KeyWidth[j] - fKeyGap, 2.0f, 0xFF3D3D3D, 0xFF3D3D3D, 0xFF999999, 0xFF999999 );
            
            /*
            if(j == 60)
            {
                float fMXGap = floor( _KeyWidth[j] * 0.25f + 0.5f );
                float fMCX = _KeyWidth[j] - fMXGap * 2.0f - fKeyGap;
                float fMY = std::max( fCurY + fTopCY - fMCX - 5.0f, fCurY + fSharpCY + 5.0f );
                //DrawRect(Ren, fCurX + fKeyGap1 + fMXGap, fMY+WinH - WinW * 82 / 1000, fMCX, fCurY + fTopCY - 5.0f - fMY, 0xFFCCCCCC,0xFFCCCCCC,0xFFCCCCCC,0xFFCCCCCC );
            }
            */
        }
        else
        {
            unsigned int c = KeyColor[j];
            unsigned short r = (c >> 16) & 0xFF;
            unsigned short g = (c >> 8) & 0xFF;
            unsigned short b = c & 0xFF;
            unsigned short r2 = r * 0.6f;
            unsigned short g2 = g * 0.6f;
            unsigned short b2 = b * 0.6f;
            
            unsigned int c_bgr = 0xFF000000 | (b << 16) | (g << 8) | r;
            unsigned int darker = 0xFF000000 | (b2 << 16) | (g2 << 8) | r2;
            
            // Draw the colored fill of the white keys
            DrawRect(Ren, fCurX + fKeyGap1, fCurY + WinH - WinW * 82 / 1000, _KeyWidth[j] - fKeyGap, fTopCY + fNearCY - 2.0f, darker, darker, c_bgr, c_bgr);
            
            // Draw the colored bottom of the white key when pressed
            DrawRect(Ren, fCurX + fKeyGap1, fCurY + fTopCY + fNearCY - 2.0f + WinH - WinW * 82 / 1000, _KeyWidth[j] - fKeyGap, 2.0f, darker, darker, darker, darker);
            
            /*
            if(j == 60)
            {
                float fMXGap = floor(_KeyWidth[j] * 0.25f + 0.5f);
                float fMCX = _KeyWidth[j] - fMXGap * 2.0f - fKeyGap;
                float fMY = std::max(fCurY + fTopCY + fNearCY - fMCX - 7.0f, fCurY + fSharpCY + 5.0f);
                //DrawRect(Ren, fCurX + fKeyGap1 + fMXGap, fMY + WinH - WinW * 82 / 1000, fMCX, fCurY + fTopCY + fNearCY - 7.0f - fMY, darker, darker, darker, darker);
            }
            */
        }
        
		//Gray edges of the white keys (No note color mixing is done)
		DrawRect(Ren, floor( fCurX + fKeyGap1 + _KeyWidth[j] - fKeyGap + 0.5f ), fCurY+WinH - WinW * 82 / 1000, fKeyGap, fTopCY + fNearCY, 0xFF000000, 0xFF999999, 0xFF999999, 0xFF000000 );
		fCurX+=_KeyWidth[j];
    }
    
    float fSharpTop = SharpRatio * 0.7f;

    fCurY = fTransitionCY + fRedCY + fSpacerCY;
    
	for(int i=75; i != 128; ++i)
    {
	    int j = KeyMap[i];
	    float fNudgeX = 0.3;
        fCurX = KeyX[j];
        const float cx = _KeyWidth[0] * SharpRatio;
        const float x = fCurX - _KeyWidth[0] * ( SharpRatio / 2.0f - fNudgeX );
        const float fSharpTopX1 = x + _KeyWidth[0] * ( SharpRatio - fSharpTop ) / 2.0f;
        const float fSharpTopX2 = fSharpTopX1 + _KeyWidth[0] * fSharpTop;
	    if(!KeyPress[j])//If the key is not pressed
        {
            // Black keys bottom end
            DrawSkew(Ren, fSharpTopX1, fCurY + fSharpCY - fNearCY+WinH - WinW * 82 / 1000, fSharpTopX2, fCurY + fSharpCY - fNearCY+WinH - WinW * 82 / 1000, x + cx, fCurY + fSharpCY+WinH - WinW * 82 / 1000, x, fCurY + fSharpCY+WinH - WinW * 82 / 1000, 0xFF404040,0xFF404040, 0xFF000000, 0xFF000000);
            
            // Left side of the black keys
            DrawSkew(Ren, fSharpTopX1, fCurY - fNearCY+WinH - WinW * 82 / 1000, fSharpTopX1, fCurY + fSharpCY - fNearCY+WinH - WinW * 82 / 1000, x, fCurY + fSharpCY+WinH - WinW * 82 / 1000, x, fCurY+WinH - WinW * 82 / 1000, 0xFF404040,0xFF404040, 0xFF000000, 0xFF000000);
            
            // Right side of the black keys
            DrawSkew(Ren, fSharpTopX2, fCurY + fSharpCY - fNearCY+WinH - WinW * 82 / 1000, fSharpTopX2, fCurY - fNearCY+WinH - WinW * 82 / 1000, x + cx, fCurY+WinH - WinW * 82 / 1000, x + cx, fCurY + fSharpCY+WinH - WinW * 82 / 1000, 0xFF404040,0xFF404040, 0xFF000000, 0xFF000000);
            
            // Bottom half gradient of the black keys
            DrawRect(Ren, fSharpTopX1, fCurY - fNearCY+WinH - WinW * 82 / 1000, fSharpTopX2 - fSharpTopX1, fSharpCY, 0xFF000000,0xFF000000,0xFF000000,0xFF000000);
            
            // Top half gradient of the black keys
            DrawSkew(Ren, fSharpTopX1, fCurY - fNearCY+WinH - WinW * 82 / 1000, fSharpTopX2, fCurY - fNearCY+WinH - WinW * 82 / 1000, fSharpTopX2, fCurY - fNearCY + fSharpCY * 0.45f+WinH - WinW * 82 / 1000, fSharpTopX1, fCurY - fNearCY + fSharpCY * 0.35f+WinH - WinW * 82 / 1000, 0xFF202020,0xFF202020,0xFF404040,0xFF404040);
            
            // Middle gradient (?)
            DrawSkew(Ren, fSharpTopX1, fCurY - fNearCY + fSharpCY * 0.35f+WinH - WinW * 82 / 1000, fSharpTopX2, fCurY - fNearCY + fSharpCY * 0.45f+WinH - WinW * 82 / 1000, fSharpTopX2, fCurY - fNearCY + fSharpCY * 0.65f+WinH - WinW * 82 / 1000, fSharpTopX1, fCurY - fNearCY + fSharpCY * 0.55f+WinH - WinW * 82 / 1000, 0xFF404040,0xFF404040, 0xFF000000, 0xFF000000);
        }
        else
        {
            const float fNewNear = fNearCY * 0.25f;
            unsigned int c = KeyColor[j];
            unsigned short r = (c >> 16) & 0xFF;
            unsigned short g = (c >> 8) & 0xFF;
            unsigned short b = c & 0xFF;
            unsigned short r1 = r * 0.5f;
            unsigned short g1 = g * 0.5f;
            unsigned short b1 = b * 0.5f;
            
            unsigned int c_bgr = 0xFF000000 | (b << 16) | (g << 8) | r;
            unsigned int darker = 0xFF000000 | (b1 << 16) | (g1 << 8) | r1;
            
            // Black keys bottom end
            DrawSkew(Ren, fSharpTopX1, fCurY + fSharpCY - fNewNear + WinH - WinW * 82 / 1000, fSharpTopX2, fCurY + fSharpCY - fNewNear + WinH - WinW * 82 / 1000, x + cx, fCurY + fSharpCY + WinH - WinW * 82 / 1000, x, fCurY + fSharpCY + WinH - WinW * 82 / 1000, c_bgr, c_bgr, darker, darker);
            
            // Left side of the black keys
            DrawSkew(Ren, fSharpTopX1, fCurY - fNewNear + WinH - WinW * 82 / 1000, fSharpTopX1, fCurY + fSharpCY - fNewNear + WinH - WinW * 82 / 1000, x, fCurY + fSharpCY + WinH - WinW * 82 / 1000, x, fCurY + WinH - WinW * 82 / 1000, c_bgr, c_bgr, darker, darker);
            
            // Right side of the black keys
            DrawSkew(Ren, fSharpTopX2, fCurY + fSharpCY - fNewNear + WinH - WinW * 82 / 1000, fSharpTopX2, fCurY - fNewNear + WinH - WinW * 82 / 1000, x + cx, fCurY + WinH - WinW * 82 / 1000, x + cx, fCurY + fSharpCY + WinH - WinW * 82 / 1000, c_bgr, c_bgr, darker, darker);
            
            // Bottom half gradient of the black keys
            DrawRect(Ren, fSharpTopX1, fCurY - fNewNear + WinH - WinW * 82 / 1000, fSharpTopX2 - fSharpTopX1, fSharpCY, darker, darker, darker, darker);
            
            // Top half gradient of the black keys
            DrawSkew(Ren, fSharpTopX1, fCurY - fNewNear + WinH - WinW * 82 / 1000, fSharpTopX2, fCurY - fNewNear + WinH - WinW * 82 / 1000, fSharpTopX2, fCurY - fNewNear + fSharpCY * 0.35f + WinH - WinW * 82 / 1000, fSharpTopX1, fCurY - fNewNear + fSharpCY * 0.25f + WinH - WinW * 82 / 1000, c_bgr, c_bgr, c_bgr, c_bgr);
            
            // Middle gradient (?)
            DrawSkew(Ren, fSharpTopX1, fCurY - fNewNear + fSharpCY * 0.25f + WinH - WinW * 82 / 1000, fSharpTopX2, fCurY - fNewNear + fSharpCY * 0.35f + WinH - WinW * 82 / 1000, fSharpTopX2, fCurY - fNewNear + fSharpCY * 0.75f + WinH - WinW * 82 / 1000, fSharpTopX1, fCurY - fNewNear + fSharpCY * 0.65f + WinH - WinW * 82 / 1000, c_bgr, c_bgr, darker, darker);
        }
    }
}

unsigned int Render::GenerateRandomColor()
{
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<unsigned int> dist(60, 255); // Ensure brightness by using a higher range
    
    // Generate bright red, green, and blue components
    unsigned int r = dist(rng);
    unsigned int g = dist(rng);
    unsigned int b = dist(rng);
    
    // Combine the RGB components into a single color value
    return (r << 16) | (g << 8) | b;
}

void Render::DrawNote(NVMidi::u16_t k, const NVnote &n, int pps)
{
    std::pair<int, int> trackChannelKey = {n.track, n.chn};
        
    auto it = trackChannelColorMap.find(trackChannelKey);
    int colorIndex;
        
    if(it == trackChannelColorMap.end())
    {
        // New track/channel combination - assign next color index
        colorIndex = trackChannelColorMap.size();
        trackChannelColorMap[trackChannelKey] = colorIndex;
    }
    else
    {
        // Already assigned - use existing index
        colorIndex = it->second;
    }
        
    if(!live_conf.is_custom_ch_colors)
    {
        if(live_conf.loop_colors)
        {
            note_color = NoteColors[colorIndex % 16];
            note_color = live_conf.channel_colors[colorIndex % 16]; // This is what allows live note color changing
        }
        else
        {
            if(colorIndex < 16)
                note_color = NoteColors[colorIndex];
            else
                note_color = GenerateRandomColor();
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
                note_color = GenerateRandomColor();
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

void Render::LoadBackgroundImage(std::string file)
{
    background_img = IMG_LoadTexture(Ren, file.c_str());
    
    if(!background_img)
    {
        Log::error("", "Failed to load background image: %s", SDL_GetError());
        return;
    }
}

void Render::ClearTrackChannelColors()
{
    trackChannelColorMap.clear();
    std::map<std::pair<int, int>, unsigned int>().swap(trackChannelColorMap); 
}

bool IsSharp(int note)
{
    int n = note % 12;
    return (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
}

void Render::DrawBackgroundGrid()
{
    for(int i = 1; i <= 127; ++i)
    {
        if(!IsSharp(i - 1) && !IsSharp(i))
        {
            float x = KeyX[i - 1] + _KeyWidth[i - 1];
            x = floorf(x + 0.5f);
            DrawRect(Ren, x - 1.0f, 0.0f, 1.8f, WinH, 0x402A2A2A, 0x601F1F1F, 0x601F1F1F, 0x402A2A2A);
        }
    }
}


void Render::HandleResize(int newWidth, int newHeight)
{
    WinW = newWidth;
    WinH = newHeight;
    
    // Keyboard resizing + re-position
    for(int i = 0; i != 128; ++i)
    {
        KeyX[i] = (i / 12 * 126 + GenKeyX[i % 12]) * WinW / 1350;
    }
   
    for(int i = 0; i != 127; ++i)
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
    BkeyW = scale(60);
    WkeyW = scale(94);
}

int Render::scale(int x)
{
    // Fixed a small note glitch LMfaoerGdfgg
    return (x * WinW + 4700) / 7330;
}