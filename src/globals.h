#ifndef GLOBALS_H
#define GLOBALS_H


#include <SDL3/SDL.h>

#include "audio/playback.h"
#include "config/config.h"
#include "config/file_dialog_state.h"
#include "mb_types.h"
#include "render/note_buffer.h"
#include "render/render.h"














inline Config::configuration              loaded_config;
inline Config::configuration              live_conf;
inline std::vector<std::string>           live_midi_list;
inline std::vector<std::string>           loaded_midi_list;
inline u32                                Playback::main_stream;
inline std::vector<Playback::AudioDevice> availableAudioDevices;
inline Render                            *RenderWin;
inline int                                _WinH;
inline f64                                Tscr;
inline f64                                vis_Tscr;
inline f64                                smooth_tick_scale = 0.0;
inline bool                               is_defaultconfig;
inline bool                               Playback::is_paused;
inline std::vector<UI::SoundfontItem>     loaded_soundfont_list;
inline FileDialogState::DirPaths          loaded_file_dialog_state;
inline FileDialogState::DirPaths          live_fd_state;
inline std::vector<NoteBuffer::Note>      note_buf;

inline Config::configuration              default_settings = {
    .bass_voice_count        = 500,
    .audio_device_index      = 1,
    .note_speed              = 4271,
    .bg_R                    = 47,
    .bg_G                    = 47,
    .bg_B                    = 47,
    .bg_A                    = 255,
    // clang-format off
    .channel_colors          = {
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
    // clang-format on
    .vel_min                 = 0,
    .vel_max                 = 32,
    .midi_index              = 0,
    .fps                     = 500,
    .builtin_ui_theme_idx    = 0,
    .audio_limiter           = true,
    .vel_filter              = false,
    .use_default_paths       = true,
    .background_image        = false,
    .no_midi_duplicates      = false,
    .no_soundfont_duplicates = false,
    .loop_colors             = false,
    .draw_vertical_lines     = true,
    .draw_measure_lines      = true,
    .vsync                   = true,
    .internal_log_buffer     = true,
    .custom_ui_theme         = false,
    .builtin_ui_theme        = true,
    .OR                      = true,
    .tick_based_playback     = false,
};


#endif
