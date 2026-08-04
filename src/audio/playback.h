#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <string>
#include <vector>

#include <bass.h>
#include <bassmidi.h>

#include "../mb_types.h"
#include "../nv_midi/list.h"
#include "../render/ui.h"


#ifdef PLATFORM_ANDROID
    #define DEFAULT_SOUND_FONT_PATH    "/data/data/com.qsp.nvpfa/files/piano_maganda.sf2"
    #define DEFAULT_GM_SOUND_FONT_PATH "/data/data/com.qsp.nvpfa/files/gm_generic.sf2"
#else
    #define DEFAULT_SOUND_FONT_PATH    "piano_maganda.sf2"
    #define DEFAULT_GM_SOUND_FONT_PATH "gm_generic.sf2"
#endif














extern NVnoteList Midi_ctx;

class Playback
{
  public:
    typedef struct
    {
        int         index;
        const char *name;
        const char *driver;
        bool        is_default;
        bool        is_enabled;
    } AudioDevice;

    static u32                      main_stream;
    static bool                     is_playback_started;
    static bool                     is_midi_loaded;
    static bool                     is_midi_loading;
    static bool                     is_midi_stream_creating;
    static bool                     playback_ended;
    static u64                      saved_position;
    static const f64                seek_amount;
    static bool                     is_paused;
    static f64                      Tplay, Tscr;

    static std::vector<AudioDevice> GetAudioOutputs();
    static void                     loadMidiFile(const std::string &midi_path);
    static void                     CloseMidi();
    static void                     LoadDefaultSoundfonts();
    static bool                     LoadEnabledSoundfonts(std::vector<UI::SoundfontItem> enabled_soundfonts);
    static void                     ReloadSoundfonts();
    static void                     updateBassVoiceCount(int voiceCount);
    static void                     seek_playback(f64 seconds);
    static void                     bassErrorHandler();
    static void                     pause();
    static void                     PlayerStateUpdate();
};
#endif