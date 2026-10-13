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
    static u64                      current_player_pos;
    static const f64                seek_amount;
    static bool                     is_paused;
    static f64                      Tplay, Tscr;
    static u64 playback_start_ns;

    static void                     Init();
    static void                     loadMidiFile(const std::string &midi_path);
    static void                     CloseMidi();
    static void                     LoadDefaultSoundfonts();
    static bool                     LoadEnabledSoundfonts(std::vector<UI::SoundfontItem> enabled_soundfonts);
    static void                     ReloadSoundfonts();
    static void                     updateVoiceCount(int voiceCount);
    static void                     seek_playback(f64 seconds);
    static void                     UpdateEndPosition();
    static void                     UpdateMidiPlayerPos();
    static bool                     IsMidiPlayerActive();
    static bool                     IsMIDIEnded();
    static void                     pause();
    static void                     PlayerStateUpdate();
    static void                     Close();
    static void                     SetNoteVelSkipping(int low_vel, int high_vel, bool enabled);
    static void                     SetMidiChunkLimit(int chunk_limit, bool enabled);
    static void                     SetAudioFrameSize(int frame_size);
    static void                     SetOverlappingNotes(bool enabled);
};

class MIDIAudioStream
{
    public:
        SDL_AudioSpec config;
        SDL_AudioStream *stream;
        SDL_AudioDeviceID *devices;

        typedef struct
        {
            int index;
            int ID;
            std::string name;
        }Device;

        static void callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount);

        MIDIAudioStream(std::string audioDrvName);
        ~MIDIAudioStream();

        std::vector<std::string> driverList;
        std::vector<Device> deviceList;

        bool isDiskDriverAvailable; // Todo
        void CreateDriverList();
        void CreateDeviceList();
        void SetAudioFilenamePath(std::string audioFName);
        void SetDevice(int devID_Index);
        void Close();
};
#endif
