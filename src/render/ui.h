#ifndef UI_H
#define UI_H


#include <string>
#include <vector>


#include <SDL3/SDL.h>
#include <imgui.h>
#include "../nv_midi/utils.h"





#ifdef PLATFORM_ANDROID
    #define IMGUI_INI_FILE_PATH "/data/data/com.qsp.nvpfa/files/imgui.ini"
#else
    #define IMGUI_INI_FILE_PATH "imgui.ini"
#endif

#define FONT_AWESOME_ICON_SIZE 24





inline std::vector<std::string> checked_soundfonts;
inline std::vector<std::string> image_files;
inline std::vector<std::string> all_image_files;
inline std::vector<SDL_Texture*> image_textures;

unsigned int ImVec4ToUInt(const ImVec4& color);
ImVec4 UIntToImVec4(unsigned int rgb);



struct LoadMidiArgs {
    std::string midi_path;
};

class UI
{
    public:
    
        struct SoundfontItem
        {
            std::string label;
            bool checked = false;
        };
        
        typedef struct
        {
            int r;
            int g;
            int b;
            int a;
        }RGBAint;
        
        static bool main_gui_window;
        static int live_note_speed;
        static int selIndex;
        static int min_velocity;
        static int max_velocity;
        static int current_audio_dev;
        static bool velocity_filter;
        static bool loop_colors;
        static bool overlap_remover;
        static bool use_bg_image;
        static bool no_midi_duplicates;
        static bool no_soundfont_duplicates;
        static bool vsync;
        static bool use_default_media_paths;
        static ImVec4 ui_chcolors[16];
        static ImVec4 clear_color;
        static RGBAint liveColor;
        static std::string last_midi_path;
        static std::string last_sf_path;
        static std::string last_midi_file;
        static std::vector<std::string> soundfont_paths;
        static std::vector<std::string> prev_images;
        
        static bool show_demo_window;
        
        static void Setup(SDL_Window *w, SDL_Renderer *r);
        static void SetDefaultTheme();
        static void Render(SDL_Renderer *r);
        static std::vector<std::string> GetCheckedSoundfonts(const std::vector<SoundfontItem>& items);
        static void UpdateWidgetValues();
        static RGBAint Frgba2Irgba(ImVec4& col);
        static ImVec4 Irgba2ImVec4(int r, int g, int b, int a);
        static unsigned int ImVec4ToUInt(const ImVec4& color);
        static ImVec4 UIntToImVec4(unsigned int rgb);
};

inline std::vector<UI::SoundfontItem> live_soundfont_list;

void Exit();
#endif