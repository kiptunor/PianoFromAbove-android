#ifndef RENDER_H
#define RENDER_H

#include <string>


#include "../mb_types.h"
#include "../nv_midi/list.h"
#include <SDL3/SDL.h>














class Render
{
  public:
    SDL_Window            *Win;
    SDL_Renderer          *Ren;
    const SDL_DisplayMode *mod;
    static const us_char   KeyMap[128];
    static us_int          note_color;
    SDL_Texture           *background_img;

    int                    TH, WinW, WinH;
    bool                   KeyPress[128];
    us_int                 KeyColor[128];

    Render();
    ~Render();

    void        clear();
    void        DrawKeyBoard();
    void        DrawNote(NVMidi::u16_t k, const NVnote &n, int pps);
    void        DrawBackgroundGrid();
    void        DrawHorizontalLines();
    void        LoadBackgroundImage(std::string file);
    void        HandleResize(int newWidth, int newHeight);
    void        CreateNote(int k, int yb, int ye, us_int c);
    static bool IsSharp(int note);
    static bool isDesktopSession();

  private:
    int BkeyW, BkeyH, WkeyW, WkeyH;
    int TW, KeyX[128];

    int scale(int x);
};

#endif
