#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <string>
#include <vector>

#include <SDL3/SDL.h>



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


#define DEFAULT_SAMPLE_RATE 48000











extern NVnoteList Midi_ctx;

class MIDIPlayer
{
  public:
    // static u32       main_stream;
    static bool      is_playback_started;
    static bool      is_midi_loaded;
    static bool      is_midi_loading;
    static bool      is_midi_stream_creating;
    static bool      playback_ended;
    static u64       saved_position;
    static u64       current_player_pos;
    static const f64 seek_amount;
    static bool      is_paused;
    static f64       Tplay, Tscr;
    static u64       playback_start_ns;

    static void      Init();
    static void      LoadMidiFile(const std::string &midi_path);
    static void      CloseMidi();
    static void      LoadDefaultSoundfonts();
    static bool      LoadEnabledSoundfonts(std::vector<UI::SoundfontItem> enabled_soundfonts);
    static void      ReloadSoundfonts();
    static void      UpdateVoiceCount(int voiceCount);
    static void      Seek_lazy(f64 seconds);
    static void      Seek(f64 seconds);
    static void      UpdateEndPosition();
    static void      SetReverbPreset(int rv_pr_idx);
    static void      UpdateMidiPos();
    static bool      IsActive();
    static bool      IsMIDIEnded();
    static f64       GetEndPos();
    static f64       GetPrerenderPos();
    static bool      IsPrerenderFinished();
    static void      Pause();
    static void      UpdateState();
    static void      Close();
    static void      SetNoteVelSkipping(int low_vel, int high_vel, bool enabled);
    static void      SetMidiChunkLimit(int chunk_limit, bool enabled);
    static void      SetAudioFrameSize(int frame_size);
    static void      SetOverlappingNotes(bool enabled);
    static void      ExportAudio(std::string fname);
    static int       GetActiveVoices();
};
#endif
