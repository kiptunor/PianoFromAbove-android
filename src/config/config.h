#ifndef CONFIG_H
#define CONFIG_H




#include <string>
#include <vector>

//#include "render/render.h"



#ifdef PLATFORM_ANDROID
    #define CONFIG_FILE_PATH "/data/data/com.qsp.nvpfa/files/settings.json"
#else
    #define CONFIG_FILE_PATH "settings.json"
#endif



class Config
{
    public:
        typedef struct
        {
            int bass_voice_count;
            int audio_device_index;
            int note_speed;
            // Deprecated
            //int window_w;
            //int window_h;
            int bg_R;
            int bg_G;
            int bg_B;
            int bg_A;
            unsigned int channel_colors[16];
            int vel_min;
            int vel_max;
            int midi_index;
            bool audio_limiter;
            bool vel_filter;
            bool use_default_paths;
            bool is_custom_ch_colors; // Only for internal use!!!
            bool use_default_colors; // Only for internal use!!!
            bool use_bg_img;
            bool no_midi_duplicates;
            bool no_soundfont_duplicates;
            bool loop_colors;
            bool draw_vertical_lines;
            bool vsync;
            //bool auto_refresh;
            bool OR; // Overlap remover
            std::string last_midi_path;
            std::string last_sf_path;
            std::string last_midi_file;
            std::string bg_img;
            std::vector<std::string> current_soundfonts;
            std::vector<std::string> extra_midi_paths;
            std::vector<std::string> extra_sf_paths;
            std::vector<std::string> extra_img_paths;
        }configuration;
        static configuration Load();
        static void Save(configuration config);
//#ifdef DEBUG
        static void PrintLoadedConfig(configuration c);
//#endif
};


inline Config::configuration default_settings =
{
    .bass_voice_count = 500,
    .audio_device_index = 0,
    .note_speed = 4271,
    .bg_R = 47,
    .bg_G = 47,
    .bg_B = 47,
    .bg_A = 255,
    .channel_colors = 
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
    },
    .vel_min = 0,
    .vel_max = 32,
    .midi_index = 0,
    .audio_limiter = true,
    .vel_filter = false,
    .use_default_paths = true,
    .use_bg_img = true,
    .no_midi_duplicates = false,
    .no_soundfont_duplicates = false,
    .loop_colors = false,
    .draw_vertical_lines = true,
    .vsync = true,
    .OR = false,
};

#endif