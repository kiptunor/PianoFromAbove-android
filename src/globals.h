#ifndef GLOBALS_H
#define GLOBALS_H


#include <SDL3/SDL.h>

#include "config/config.h"
#include "mb_types.h"
#include "audio/playback.h"
#include "config/file_dialog_state.h"
//#include "config/soundfont_list.h"



inline Config::configuration loaded_config;
inline Config::configuration live_conf;
inline std::vector<std::string> live_midi_list;
inline std::vector<std::string> loaded_midi_list;
inline u32 Playback::main_stream;
inline std::vector<Playback::AudioDevice> availableAudioDevices;
inline int _WinH;
inline f64 Tscr;
inline SDL_Mutex *bass_mutex = nullptr;
inline bool is_defaultconfig;
inline bool Playback::is_paused;
inline std::vector<UI::SoundfontItem> loaded_soundfont_list;
inline FileDialogState::DirPaths loaded_file_dialog_state;
inline FileDialogState::DirPaths live_fd_state;


#endif