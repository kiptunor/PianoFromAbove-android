#ifndef UI_H
#define UI_H


#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "../mb_types.h"














#ifdef PLATFORM_ANDROID
    #define IMGUI_INI_FILE_PATH "/data/data/com.qsp.nvpfa/files/imgui.ini"
#else
    #define IMGUI_INI_FILE_PATH "imgui.ini"
#endif
// #define PLATFORM_ANDROID
#ifdef PLATFORM_ANDROID
    #define FONT_AWESOME_ICON_SIZE 45
    #define UI_FONT_SIZE           55.0f
#else
    #define FONT_AWESOME_ICON_SIZE 18
    #define UI_FONT_SIZE           22.0f
#endif






// #undef PLATFORM_ANDROID







inline std::vector<std::string>   checked_soundfonts;
inline std::vector<std::string>   image_files;
inline std::vector<std::string>   all_image_files;
inline std::vector<SDL_Texture *> image_textures;

us_int                            ImVec4ToUInt(const ImVec4 &color);
ImVec4                            UIntToImVec4(us_int rgb);

class UI
{
  public:
    struct SoundfontItem
    {
        std::string label;
        bool        checked = false;
    };

    typedef struct
    {
        int r;
        int g;
        int b;
        int a;
    } RGBAint;

    static bool                     main_gui_window;
    static int                      live_note_speed;
    static int                      fps;
    static int                      selected_midi_index;
    static int                      min_velocity;
    static int                      max_velocity;
    static int                      current_audio_dev;
    static bool                     velocity_filter;
    static bool                     loop_colors;
    static bool                     overlap_remover;
    static bool                     use_bg_image;
    static bool                     show_full_path_lost_midis;
    static bool                     show_full_path_lost_soundfonts;
    static bool                     no_midi_duplicates;
    static bool                     vertical_lines;
    static bool                     draw_measure_lines;
    static bool                     no_soundfont_duplicates;
    static bool                     background_image;
    static bool                     internal_logging;
    static bool                     log_to_file;
    static bool                     vsync;
    static bool                     use_default_media_paths;
    static bool                     ui_theming;
    static ImVec4                   ui_chcolors[16];
    static ImVec4                   clear_color;
    static RGBAint                  liveColor;
    static std::string              last_midi_path;
    static std::string              last_sf_path;
    static std::string              last_midi_file;
    static std::vector<std::string> soundfont_paths;
    static std::vector<std::string> prev_images;

    static bool                     show_demo_window;

    static void                     Setup(int graphics_backend);
    static void                     SetMoonlightTheme();
    static void                     SetSilvanaTheme();
    // Sloth Player's themes
    static void                     SetNeonAbyssTheme();
    static void                     SetCrimsonAzureTheme();
    static void                     SetArcticHorizonTheme();
    static void                     SetCyberpunkTheme();
    static void                     SetBuiltinTheme(int idx);

    static void                     Render(SDL_Renderer *r);
    static void                     UpdateWidgetValues();
    static ImVec4                   Irgba2ImVec4(int r, int g, int b, int a);
    static ImVec4                   UIntToImVec4(u_int rgb);
};

inline std::vector<UI::SoundfontItem> live_soundfont_list;

// void                                  Exit();
#endif