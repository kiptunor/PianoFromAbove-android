#ifndef RENDER_H
#define RENDER_H



#include <map>
#include <SDL3/SDL.h>
#include "../nv_midi/list.h"






extern const unsigned char KeyMap[128];
inline bool is_image_loaded;


inline unsigned int NoteColors[]= 
{
    0x3366FF,
    0xFF7E33,
    0x33FF66,
    0xFF3381,
    0x33FFFF,
    0xE433FF,
    0x99FF33,
    0x4B33FF,
    0xFFCC33,
    0x33B4FF,
    0xFF3333,
    0x33FFB1,
    0xFF33CC,
    0x4EFF33,
    0x9933FF,
    0xE7FF33
};

struct PairHash
{
    template <typename T1, typename T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const
    {
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);
        return hash1 ^ (hash2 << 1); // Combine the two hashes
    }
};

inline unsigned int note_color;

class Render {
public:

    SDL_Window   *Win;
    SDL_Renderer *Ren;
	const SDL_DisplayMode *mod;
	SDL_Texture *background_img;
	
    int   TH, WinW, WinH;
    bool  KeyPress[128];
    unsigned int KeyColor[128];
    
    Render();
    ~Render();
    
    
    void clear();

    void DrawKeyBoard();
    unsigned int GenerateRandomColor();
    void DrawNote(NVMidi::u16_t k, const NVnote &n, int pps);
    void ClearTrackChannelColors();
    void DrawBackgroundGrid();
    void LoadBackgroundImage(std::string file);
    void HandleResize(int newWidth, int newHeight);

    void CreateNote(int k, int yb, int ye, unsigned int c);

private:
    //std::unordered_map<std::pair<int, int>, unsigned int, PairHash> trackChannelColorMap;
    std::map<std::pair<int, int>, unsigned int> trackChannelColorMap;

    //SDL_Texture *Bk0, *Bk1, *Wk, *note;
    SDL_Surface *colors;

    int BkeyW, BkeyH, WkeyW, WkeyH;
    int TW, TX[11], KeyX[128];

    int scale(int x);
};

inline Render *RenderWin;

#endif