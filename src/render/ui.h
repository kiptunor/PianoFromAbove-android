#ifndef UI_H
#define UI_H

#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "../mb_types.h"
#include "../audio/midi_audio_stream.h"












#ifdef PLATFORM_ANDROID
    #define IMGUI_INI_FILE_PATH "/data/data/com.qsp.nvpfa/files/imgui.ini"
#else
    #define IMGUI_INI_FILE_PATH "imgui.ini"
#endif
//#define PLATFORM_ANDROID
#ifdef PLATFORM_ANDROID
    #define FONT_AWESOME_ICON_SIZE 45
    #define UI_FONT_SIZE           55.0f
#else
    #define FONT_AWESOME_ICON_SIZE 18
    #define UI_FONT_SIZE           22.0f
#endif






//#undef PLATFORM_ANDROID









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

    static int render_mode;

    std::vector<UI::SoundfontItem> live_soundfont_list;

    bool                     main_gui_window;
    int                      live_note_speed;
    int                      fps;
    int                      selected_midi_index;
    int                      selected_reverb_preset;
    int                      min_velocity;
    int                      max_velocity;
    int                      current_audio_dev;
    int                      current_audio_drv;
    int                      midi_chunk_size;
    int                      audio_frame_size;
    f32                      current_midi_pos_sl;
    bool                     velocity_filter;
    bool                     loop_colors;
    bool                     overlap_remover;
    bool                     use_bg_image;
    bool                     show_full_path_lost_midis;
    bool                     show_full_path_lost_soundfonts;
    bool                     no_midi_duplicates;
    bool                     vertical_lines;
    bool                     meas_lines;
    bool                     no_soundfont_duplicates;
    bool                     background_image;
    bool                     internal_logging;
    bool                     log_to_file;
    bool                     vsync;
    bool                     overlapping_notes;
    bool                     midi_chunk_limiter;
    bool                     use_default_media_paths;
    bool                     ui_theming;
    bool                     overlay_stats_window;
    ImVec4                   ui_chcolors[16];
    ImVec4                   clear_color;
    RGBAint                  liveColor;
    std::string              last_midi_path;
    std::string              last_sf_path;
    std::string              last_midi_file;
    std::vector<std::string> soundfont_paths;
    std::vector<std::string> prev_images;

    bool                     show_demo_window;

    UI(int graphics_mode);
    ~UI();

    //static void                     Setup(int graphics_backend);
    

    void Render();
    void UpdateWidgetValues();
    static ImVec4 Irgba2ImVec4(int r, int g, int b, int a);
    static ImVec4 UIntToImVec4(u_int rgb);

  private:
      std::vector<std::string>   checked_soundfonts;
      std::vector<std::string>   image_files;
      std::vector<std::string>   all_image_files;
      std::vector<SDL_Texture *> image_textures;

      us_int ImVec4ToUInt(const ImVec4 &color);

      void SetupIconFonts();
      std::string FilenameOnly(const std::string &path);
      bool searchDuplicatedMidiItem(const std::vector<std::string> &items, const std::string &item);
      bool searchDuplicatedSoundfontItem(const std::vector<UI::SoundfontItem> &items, const std::string &item);
      void RenderMidiList(const std::vector<std::string> &items, int &selectedIndex, std::string find_item);
      std::vector<std::string> GetCheckedSoundfonts(const std::vector<UI::SoundfontItem> &items);
      void RenderSoundfontsPathsList(const std::vector<std::string> &items, int &selectedIndex);
      void moveSoundfont(int index, int direction);

      void ConstrainWindowMove(const char *windowName);
      void AndroidUIRescale();
      
      void ShowAudioDeviceList(const std::vector<MIDIAudioStream::Device> &audioDevices);
      void ShowAudioDriverList(const std::vector<std::string> &audioDrivers);
      void RenderSoundfontList(std::vector<UI::SoundfontItem> &items, std::string find_item);

      void                     SetMoonlightTheme();
      void                     SetSilvanaTheme();
      // Sloth Player's themes
      void                     SetNeonAbyssTheme();
      void                     SetCrimsonAzureTheme();
      void                     SetArcticHorizonTheme();
      void                     SetCyberpunkTheme();
      void                     SetBuiltinTheme(int idx);
};

//inline std::vector<UI::SoundfontItem> live_soundfont_list;

//void                                  Exit();
#endif