#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

#ifdef PLATFORM_ANDROID
    #define CONFIG_FILE "/data/data/com.qsp.nvpfa/files/settings.json"
#else
    #define CONFIG_FILE  "settings.json"
    #define CONFIG_DIR   "/.config/npfa"
    #define CONFIG_LISTS "/.cache/npfa"
#endif














class Config
{
  public:
    typedef struct
    {
        int                      bass_voice_count;
        int                      audio_device_index;
        int                      note_speed;
        int                      bg_R;
        int                      bg_G;
        int                      bg_B;
        int                      bg_A;
        unsigned int             channel_colors[16];
        int                      vel_min;
        int                      vel_max;
        int                      midi_index;
        int                      fps;
        int                      builtin_ui_theme_idx;
        bool                     audio_limiter;
        bool                     vel_filter;
        bool                     use_default_paths;
        bool                     is_custom_ch_colors;     // Only for internal use!!!
        bool                     use_default_colors;      // Only for internal use!!!
        bool                     dont_show_missing_files; // Only for internal use!!!
        bool                     background_image;
        bool                     no_midi_duplicates;
        bool                     no_soundfont_duplicates;
        bool                     loop_colors;
        bool                     draw_vertical_lines;
        bool                     draw_measure_lines;
        bool                     vsync;
        bool                     internal_log_buffer;
        bool                     log_to_file;
        bool                     custom_ui_theme;
        bool                     builtin_ui_theme;
        bool                     OR; // Overlap remover
        std::string              last_ccol_file_path;
        std::string              background_image_path;
        std::string              ui_theme_file_path;
        std::vector<std::string> current_soundfonts;
        std::vector<std::string> extra_midi_paths;
        std::vector<std::string> extra_img_paths;
        bool                     tick_based_playback;
    } configuration;

    static configuration Load();
    static void          Save(configuration config);
    static std::string   config_path;
    static unsigned int  hexToUInt(const std::string &hex);
};

#endif
