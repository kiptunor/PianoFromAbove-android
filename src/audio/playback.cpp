#include <atomic>
#include <filesystem>
#include <sstream>
#include <thread>
#include <vector>


#include <bass.h>
#include <bassmidi.h>


#include "../config/midi_list.h"
#include "../globals.h"
#include "../logger.h"
#include "../nv_midi/list.h"
#include "../render/note_buffer.h"
#include "audio_effects.h"
#include "playback.h"














f64               Playback::Tplay                   = 0.0;
bool              Playback::preRollActive           = false;
bool              Playback::is_midi_loaded          = false;
bool              Playback::is_playback_started     = false;
bool              Playback::playback_ended          = false;
bool              Playback::is_midi_loading         = false;
bool              Playback::is_midi_stream_creating = false;
u64               Playback::saved_position          = 0;
const f64         Playback::seek_amount             = 3.0;
f64               Playback::Tscr;
std::chrono::steady_clock::time_point Playback::clock_start;
double                               Playback::clock_base_Tplay = 0.0;
bool                                 Playback::clock_running    = false;
uint64_t                             Playback::tick_position   = 0;
std::atomic<bool> is_midi_loaded_fn          = false;
std::atomic<bool> is_midi_loading_fn         = false;
std::atomic<bool> is_midi_stream_creating_fn = false;
std::string       last_midi_path;







void              LoadMidi(const std::string &midi_path)
{
    if(!Midi_ctx.start_parse(midi_path.c_str()))
    {
        std::ostringstream temp_msg;
        temp_msg << "Failed to load '" << midi_path << "'";
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "MIDI Loading Error!!!!!", temp_msg.str().c_str(), nullptr);
        return;
    }

    is_midi_loading_fn.store(true, std::memory_order_release);

    // How tf did I missed this for so long ??? :skull:
    if(live_conf.OR)
        Midi_ctx.OR();


    // - - - - [Create new MIDI stream] - - - -


    is_midi_stream_creating_fn.store(true, std::memory_order_release); // Midi stream is creating
    Log::debug("Creating MIDI Stream...");
    // Honestly idfk which one is better
    // Playback::main_stream = BASS_StreamCreateFile(0, midi_path.c_str(), 0, 0, BASS_SAMPLE_FLOAT | BASS_STREAM_PRESCAN | BASS_STREAM_DECODE); // On heavy load it caues longer sutters
    Playback::main_stream = BASS_MIDI_StreamCreateFile(FALSE, midi_path.c_str(), 0, 0, BASS_SAMPLE_FLOAT, 1); // While this 1 causes many short stutters. Absolute garbage
    BASS_ChannelSetDSP(Playback::main_stream, &dsp_limiter, 0, 0);

    if(!Playback::LoadEnabledSoundfonts(live_soundfont_list))
        Playback::LoadDefaultSoundfonts();

    BASS_ChannelSetAttribute(Playback::main_stream, BASS_ATTRIB_MIDI_VOICES, live_conf.bass_voice_count);
    BASS_ChannelSetAttribute(Playback::main_stream, BASS_ATTRIB_SRC, 30);
    BASS_ChannelSetAttribute(Playback::main_stream, BASS_ATTRIB_MIDI_CHANS, 16);
    if(live_conf.vel_filter == true)
        BASS_MIDI_StreamSetFilter(Playback::main_stream, 0, reinterpret_cast<BOOL (*)(HSTREAM, int, BASS_MIDI_EVENT *, BOOL, void *)>(filter), nullptr);

    is_midi_stream_creating_fn.store(false, std::memory_order_release); // Midi stream was created

    Log::debug("MIDI Stream created.");

    is_midi_loading_fn.store(false, std::memory_order_release);
    is_midi_loaded_fn.store(true, std::memory_order_release);
}


void Playback::LoadDefaultSoundfonts()
{

    std::vector<BASS_MIDI_FONT> fontSet;
    u32                         Sf1;
    u32                         Sf2;
#ifndef NON_ANDROID
    // std::string default_sf_path    = FileHelpers::GetFilePathA("piano_maganda.sf2", "rb"); // Todo
    // std::string default_gm_sf_path = FileHelpers::GetFilePathA("gm_generic.sf2", "rb");
    Sf1 = BASS_MIDI_FontInit(DEFAULT_GM_SOUND_FONT_PATH, 0);
    Sf2 = BASS_MIDI_FontInit(DEFAULT_SOUND_FONT_PATH, 0);
#else
    Sf1 = BASS_MIDI_FontInit(DEFAULT_GM_SOUNDFONT, 0);
    Sf2 = BASS_MIDI_FontInit(DEFAULT_SOUNDFONT, 0);
#endif

    fontSet.push_back({ Sf2, -1, 0 }); // override preset 40 (Violin) in bank 0
    fontSet.push_back({ Sf1, -1, 0 });

    if(!fontSet.empty())
        BASS_MIDI_StreamSetFonts(Playback::main_stream, fontSet.data(), fontSet.size());
}

bool Playback::LoadEnabledSoundfonts(std::vector<UI::SoundfontItem> enabled_soundfonts)
{
    std::vector<BASS_MIDI_FONT> fontSet;

    bool                        is_enabled_sf_available = false;

    for(const auto &soundfont : enabled_soundfonts) // Iterate through all enabled soundfonts
    {
        if(soundfont.checked)
        {
            HSOUNDFONT Sf = BASS_MIDI_FontInit(soundfont.label.c_str(), 0);
            if(Sf)
            {
                // BASS_MIDI_FontSetVolume(Sf, 0.15);
                BASS_MIDI_FONT font = { Sf, -1, 0 }; // Set the soundfont context, preset and bank
                fontSet.push_back(font);
            }
            is_enabled_sf_available = true;
        }
    }

    if(!fontSet.empty())
        BASS_MIDI_StreamSetFonts(Playback::main_stream, fontSet.data(), fontSet.size()); // Load soundfonts

    return is_enabled_sf_available;
}

void Playback::ReloadSoundfonts()
{
    if(!Playback::main_stream || !BASS_ChannelIsActive(Playback::main_stream))
        return;

    u64  position    = BASS_ChannelGetPosition(Playback::main_stream, BASS_POS_BYTE);
    bool was_playing = !is_paused;

    // I spent nearly 2 hours trying to fix playback reset when switching soundfonts in puased state.
    // Fuck you BASS
    // BASS_ChannelStop(Playback::main_stream); // Don't stop the playback here. It resets the entire midi playback.


    if(!LoadEnabledSoundfonts(live_soundfont_list))
        LoadDefaultSoundfonts();

    BASS_ChannelSetPosition(Playback::main_stream, position, BASS_POS_BYTE);

    if(was_playing)
    {
        BASS_ChannelPlay(Playback::main_stream, FALSE);
        is_paused = false;
    }
    else
        is_paused = true;
}

void Playback::updateBassVoiceCount(int voiceCount)
{
    if(Playback::main_stream && BASS_ChannelIsActive(Playback::main_stream))
    {
        // Update the voice count for the current stream
        BASS_ChannelSetAttribute(Playback::main_stream, BASS_ATTRIB_MIDI_VOICES, voiceCount);

        // Update the configuration
        live_conf.bass_voice_count = voiceCount;

        // Log::info("Voice count updated to: %d");
    }
}

void Playback::loadMidiFile(const std::string &midi_path)
{
    preRollActive = true;
    std::thread midi_loading_thread(LoadMidi, midi_path);


    if(!std::filesystem::exists(midi_path))
    {
        Log::error("MIDI File does not exists ! '%s'", midi_path.c_str());
        return;
    }

    // Parse new MIDI file on a separate thread
    midi_loading_thread.detach();

    is_midi_loading = is_midi_loading_fn.load();
    last_midi_path  = midi_path;
}

void Playback::CloseMidi()
{
    // Stop current playback and free resources
    if(Playback::main_stream)
    {
        BASS_ChannelStop(Playback::main_stream);
        BASS_StreamFree(Playback::main_stream);
        Playback::main_stream = 0;
    }

    // Reset note lists
    for(int i = 0; i < 128; ++i)
    {
        Midi_ctx.Note_list[i].clear();
        std::list<NVnote>().swap(Midi_ctx.Note_list[i]); // Shrink the capacity so the visualization performace is the same when playing new midis
    }


    // Clear previous channel / track colors
    NoteBuffer::ClearTrackChannelColors();
    is_midi_loaded = false;
}

void Playback::PlayerStateUpdate()
{
    is_midi_stream_creating = is_midi_stream_creating_fn.load();
    if(is_midi_loaded_fn.exchange(false))
    {

        Playback::is_midi_loaded = true;

        // Start playback (mute before play to prevent audio blip)
        if(preRollActive)
            BASS_ChannelSetAttribute(Playback::main_stream, BASS_ATTRIB_VOL, 0.0f);
        BASS_ChannelPlay(Playback::main_stream, 1);
        if(preRollActive)
            BASS_ChannelPause(Playback::main_stream);

        Log::debug("Player started.");

        // Reset playback variables
        clock_running      = false;
        clock_base_Tplay   = 0.0;
        Tplay              = 0.0;
        tick_position      = 0;
        is_paused          = false;
        playback_ended           = false; // Allow the playback to start with the audio playback

        // Update current midi path
        MidiList::last_midi_file = last_midi_path;
    }
}

void Playback::seek_playback(f64 seconds)
{
    if(!is_midi_loaded || playback_ended) // Without cheking if playback_ended bass can start playing without midi visualization
        return;


    u64 current_byte_pos = BASS_ChannelGetPosition(Playback::main_stream, BASS_POS_BYTE);
    f64 current_time     = BASS_ChannelBytes2Seconds(Playback::main_stream, current_byte_pos);
    f64 new_time         = current_time + seconds;

    // Ensure we don't seek before the beginning
    if(new_time < 0)
        new_time = 0;

    // Convert back to bytes and set position
    u64 new_pos = BASS_ChannelSeconds2Bytes(Playback::main_stream, new_time);
    BASS_ChannelSetPosition(Playback::main_stream, new_pos, BASS_POS_BYTE);

    // Update our playback time
    Tplay = BASS_ChannelBytes2Seconds(Playback::main_stream, new_pos);
    clock_base_Tplay = Tplay;
    clock_running    = !is_paused;  // Don't start the clock if we're paused
    if(!is_paused)
        clock_start = std::chrono::steady_clock::now();
    tick_position    = Midi_ctx.secondsToTick(Tplay);
    smooth_tick_scale = 0;

    Log::debug("seek: Tplay=%.3f, clock_running=%d, is_paused=%d", Tplay, (int)clock_running, (int)is_paused);

    // When seeking backwards, reload the note data
    if(seconds < 0)
    {
        // Clear the note list
        for(int i = 0; i < 128; ++i)
            Midi_ctx.Note_list[i].clear();


        // Seek the list to the new position and update
        Midi_ctx.list_seek(Tplay);
        Midi_ctx.update_to(Tplay + Tscr);
    }

    if(playback_ended)
    {
        // If at the end and trying to seek back, restart playback from desired position
        f64 new_time = Tplay + (-seek_amount);
        if(new_time < 0)
            new_time = 0;

        u64 new_pos = BASS_ChannelSeconds2Bytes(Playback::main_stream, new_time);
        BASS_ChannelSetPosition(Playback::main_stream, new_pos, BASS_POS_BYTE);
        BASS_ChannelPlay(Playback::main_stream, FALSE);

        // Reset visualization properly
        for(int i = 0; i < 128; ++i)
            Midi_ctx.Note_list[i].clear();


        // Update time and reset flags
        Tplay = new_time;
        clock_base_Tplay = new_time;
        clock_start      = std::chrono::steady_clock::now();
        clock_running    = true;
        tick_position    = Midi_ctx.secondsToTick(new_time);
        smooth_tick_scale = 0;
        Midi_ctx.list_seek(Tplay);
        playback_ended = false;
        is_paused      = false;

        Log::debug("seek(restart): Tplay=%.3f, clock_running=%d", Tplay, (int)clock_running);
    }
}

void Playback::pause()
{
    if(!is_midi_loaded)
        return;

    if(is_paused)
    {
        BASS_ChannelPlay(main_stream, false);
        is_paused         = false;
        clock_base_Tplay  = Tplay;
        clock_start       = std::chrono::steady_clock::now();
        clock_running     = true;
        Log::debug("resume: Tplay=%.3f, clock_base=%.3f", Tplay, clock_base_Tplay);
    }
    else
    {
        saved_position = BASS_ChannelGetPosition(main_stream, BASS_POS_BYTE);
        BASS_ChannelPause(main_stream);
        clock_running      = false;
        is_paused          = true;
        Log::debug("pause: Tplay=%.3f, saved_pos=%llu", Tplay, (unsigned long long)saved_position);
    }

    if(playback_ended)
    {
        // If at the end and we press space, restart from beginning
        ReloadSoundfonts(); // Useful for when chaning soundfonts after playback ended
        BASS_ChannelSetPosition(main_stream, 0, BASS_POS_BYTE);
        BASS_ChannelSetAttribute(Playback::main_stream, BASS_ATTRIB_MIDI_VOICES, live_conf.bass_voice_count);
        BASS_ChannelSetAttribute(Playback::main_stream, BASS_ATTRIB_SRC, 30);
        BASS_ChannelSetAttribute(Playback::main_stream, BASS_ATTRIB_MIDI_CHANS, 16);

        BASS_ChannelPlay(main_stream, FALSE);
        Tplay              = 0.0;
        clock_running      = false;
        clock_base_Tplay   = 0.0;
        tick_position      = 0;
        playback_ended     = false;
        is_paused          = false;

        // Reset visualization state
        for(int i = 0; i < 128; ++i)
            Midi_ctx.Note_list[i].clear();

        Midi_ctx.list_seek(0);
    }
}

void Playback::updateTickClock()
{
    if(is_paused || playback_ended || !is_playback_started || preRollActive)
        return;

    // Derive tick position directly from Tplay via the tempo cache,
    // matching PianoFromAbove's GetCurrentTick() approach
    tick_position = Midi_ctx.secondsToTick(Tplay);
}

f64 Playback::GetTotalTime()
{
    if(!main_stream || !is_midi_loaded)
        return 0.0;
    u64 len = BASS_ChannelGetLength(main_stream, BASS_POS_BYTE);
    return BASS_ChannelBytes2Seconds(main_stream, len);
}

std::vector<Playback::AudioDevice> Playback::GetAudioOutputs()
{
    std::vector<Playback::AudioDevice> res;
    BASS_DEVICEINFO                    dev_info;
    int                                deviceIndex = 0;

    while(BASS_GetDeviceInfo(deviceIndex, &dev_info))
    {
        deviceIndex++;
        res.push_back({ deviceIndex, dev_info.name, dev_info.driver, (dev_info.flags & BASS_DEVICE_DEFAULT) ? true : false, (dev_info.flags & BASS_DEVICE_ENABLED) ? true : false });
    }

    return res;
}

void Playback::bassErrorHandler()
{
    int                error_code = BASS_ErrorGetCode();
    std::ostringstream msg_str;

    switch(error_code)
    {
    case BASS_ERROR_DEVICE:
        msg_str << "Device index is invalid\n";
        break;
    case BASS_ERROR_ALREADY:
        msg_str << "BASS Already initialized\n";
        break;
    case BASS_ERROR_DRIVER:
        msg_str << "Unavailable device driver\n";
        break;
    case BASS_ERROR_FORMAT:
        msg_str << "Unsupported format by device\n";
        break;
    case BASS_ERROR_MEM:
        msg_str << "Insufficient memory\n";
        break;
    case BASS_ERROR_NO3D:
        msg_str << "Failed to initialize 3D support\n";
        break;
    case BASS_ERROR_UNKNOWN:
        msg_str << "Unknown error occured!!!\n";
        break;
    default:
        msg_str << "Unhandled error code!!!\n";
        break;
    }
    if(Render::isDesktopSession())
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bass Init Error", msg_str.str().c_str(), NULL);
    else
        Log::critical("Failed to initialize BASS: %s", msg_str.str().c_str());
}