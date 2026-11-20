#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <string>
#include <vector>
#include <atomic>

#include <SDL3/SDL.h>

#include <kasaria.h>



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
    private:
        std::atomic<bool> is_midi_loaded_fn          = false;
        std::atomic<bool> is_midi_loading_fn         = false;
        std::atomic<bool> is_midi_stream_creating_fn = false;
        std::string       last_midi_path;
        u64               playback_start_ns;
        void LoadMidi(const std::string &midi_path);


    public:
        // static u32       main_stream;
        Kasaria *midi_synth_ctx = nullptr;
        bool      is_playback_started;
        bool      is_midi_loaded;
        bool      is_midi_loading;
        bool      is_midi_stream_creating;
        bool      playback_ended;
        u64       saved_position;
        u64       current_player_pos;
        f64       seek_amount;
        bool      is_paused;
        f64       Tplay, Tscr;
        //u64       playback_start_ns;

        MIDIPlayer();
        ~MIDIPlayer();

        void      Init();
        void      LoadMidiFile(const std::string &midi_path);
        void      CloseMidi();
        void      LoadDefaultSoundfonts();
        bool      LoadEnabledSoundfonts(std::vector<UI::SoundfontItem> enabled_soundfonts);
        void      ReloadSoundfonts();
        void      UpdateVoiceCount(int voiceCount);
        void      Seek_lazy(f64 seconds);
        void      Seek(f64 seconds);
        void      UpdateEndPosition();
        void      SetReverbPreset(int rv_pr_idx);
        void      UpdateMidiPos();
        bool      IsActive();
        bool      IsMIDIEnded();
        f64       GetEndPos();
        f64       GetPrerenderPos();
        bool      IsPrerenderFinished();
        void      Pause();
        void      UpdateState();
        void      Close();
        void      SetNoteVelSkipping(int low_vel, int high_vel, bool enabled);
        void      SetMidiChunkLimit(int chunk_limit, bool enabled);
        void      SetAudioFrameSize(int frame_size);
        void      SetOverlappingNotes(bool enabled);
        void      ExportAudio(std::string fname);
        int       GetActiveVoices();
};
#endif
