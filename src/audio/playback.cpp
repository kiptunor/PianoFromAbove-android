#include <atomic>
#include <filesystem>
#include <sstream>
#include <thread>
#include <vector>




#include <kasaria.h>


#include "../config/midi_list.h"
#include "../globals.h"
#include "../logger.h"
#include "../nv_midi/list.h"
#include "../render/note_buffer.h"
#include "audio_effects.h"
#include "playback.h"








Kasaria *midi_synth_ctx;





f64               Playback::Tplay                   = 0.0;
bool              Playback::is_midi_loaded          = false;
bool              Playback::is_playback_started     = false;
bool              Playback::playback_ended          = false;
bool              Playback::is_midi_loading         = false;
bool              Playback::is_midi_stream_creating = false;
u64               Playback::saved_position          = 0;
const f64         Playback::seek_amount             = 3.0;
f64               Playback::Tscr;
std::atomic<bool> is_midi_loaded_fn          = false;
std::atomic<bool> is_midi_loading_fn         = false;
std::atomic<bool> is_midi_stream_creating_fn = false;
std::string       last_midi_path;
u64 Playback::playback_start_ns;





void Playback::Init()
{
    midi_synth_ctx = ksr_init();

    ksr_set_fast_decay(midi_synth_ctx, true);
    ksr_set_antialiasing(midi_synth_ctx, true);
    ksr_set_sample_rate(midi_synth_ctx, 48000); // Optional
    
    // Skip notes with velocities in between the low and high specified threasholds
    // And also enable the filter
    ksr_set_note_velocity_skipping(midi_synth_ctx, 0, 20, true);

    
    ksr_set_max_voices(midi_synth_ctx, 5000); // How many voices the synth can use

    ksr_init_audio(midi_synth_ctx, INTERNAL_MIDI_PLAYER);
}

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
    /*
    if(live_conf.vel_filter == true)
        BASS_MIDI_StreamSetFilter(Playback::main_stream, 0, reinterpret_cast<BOOL (*)(HSTREAM, int, BASS_MIDI_EVENT *, BOOL, void *)>(filter), nullptr);
        */
        
    ksr_load_midi_file(midi_synth_ctx, midi_path.c_str());
    is_midi_stream_creating_fn.store(false, std::memory_order_release); // Midi stream was created

    Log::debug("MIDI Stream created.");

    is_midi_loading_fn.store(false, std::memory_order_release);
    is_midi_loaded_fn.store(true, std::memory_order_release);
}


void Playback::LoadDefaultSoundfonts()
{

#ifndef NON_ANDROID
 
    ksr_load_soundfont_file(midi_synth_ctx, DEFAULT_GM_SOUND_FONT_PATH, true);
    ksr_load_soundfont_file(midi_synth_ctx, DEFAULT_SOUND_FONT_PATH, true);
#else
// Todo
#endif
}

bool Playback::LoadEnabledSoundfonts(std::vector<UI::SoundfontItem> enabled_soundfonts)
{
    bool                        is_enabled_sf_available = false;

    for(const auto &soundfont : enabled_soundfonts) // Iterate through all enabled soundfonts
    {
        if(soundfont.checked)
        {
            ksr_load_soundfont_file(midi_synth_ctx, soundfont.label.c_str(), true);
            is_enabled_sf_available = true;
        }
    }

    return is_enabled_sf_available;
}

void Playback::ReloadSoundfonts()
{
    if(!Playback::main_stream || !ksr_is_midi_player_active(midi_synth_ctx))
        return;

    f64  position    = ksr_get_midi_player_pos(midi_synth_ctx);
    bool was_playing = !is_paused;

   
    //ksr_set_midi_player_pos(midi_synth_ctx, position);
    long ms = (long)(position * 1000);
    ksr_seek_midi(midi_synth_ctx, ms);

    if(was_playing)
    {
        ksr_pause_midi(midi_synth_ctx);
        is_paused = false;
    }
    else
        is_paused = true;
}

void Playback::updateBassVoiceCount(int voiceCount)
{
    //if(Playback::main_stream && BASS_ChannelIsActive(Playback::main_stream))
    //{
        ksr_set_max_voices(midi_synth_ctx, voiceCount);

        // Update the configuration
        live_conf.bass_voice_count = voiceCount;

        // Log::info("Voice count updated to: %d");
    //}
}

void Playback::loadMidiFile(const std::string &midi_path)
{
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
    
    ksr_pause_midi(midi_synth_ctx);
    is_paused = false;
    
    ksr_unload_midi(midi_synth_ctx);

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

        // Start playback
        ksr_play_midi(midi_synth_ctx, 0);

        Log::debug("Player started.");

        // Reset playback variables
        Tplay                    = 0.0;
        is_paused                = false;
        playback_ended           = false; // Allow the playback to start with the audio playback

        // Update current midi path
        MidiList::last_midi_file = last_midi_path;
    }
}

void Playback::seek_playback(f64 seconds)
{
    if(!is_midi_loaded || playback_ended) // Without cheking if playback_ended bass can start playing without midi visualization
        return;


    // seconds is a relative delta → absolute target in ms
    long target_ms = ksr_get_current_time(midi_synth_ctx) + (long)(seconds * 1000);
    ksr_seek_midi(midi_synth_ctx, target_ms);
    
    // Update our playback time
    Tplay = ksr_get_midi_player_pos(midi_synth_ctx);

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

        ksr_seek_midi(midi_synth_ctx, 0);

        // Reset visualization properly
        for(int i = 0; i < 128; ++i)
            Midi_ctx.Note_list[i].clear();


        // Update time and reset flags
        Tplay = new_time;
        Midi_ctx.list_seek(Tplay);
        playback_ended = false;
        is_paused      = false;
    }
}

void Playback::pause()
{
    if(!is_midi_loaded)
        return;

    Playback::is_paused = !Playback::is_paused;
    ksr_pause_midi(midi_synth_ctx);
    if(playback_ended)
    {
        // If at the end and we press space, restart from beginning
        ReloadSoundfonts(); // Useful for when chaning soundfonts after playback ended

        ksr_seek_midi(midi_synth_ctx, 0);

       // BASS_ChannelPlay(main_stream, FALSE);
        ksr_play_midi(midi_synth_ctx, 0);
        Tplay          = 0.0;
        playback_ended = false;
        is_paused      = false;

        // Reset visualization state
        for(int i = 0; i < 128; ++i)
            Midi_ctx.Note_list[i].clear();

        Midi_ctx.list_seek(0);
    }
}

void Playback::UpdateEndPosition()
{
    saved_position = ksr_get_midi_player_pos(midi_synth_ctx);
}

void Playback::UpdateMidiPlayerPos()
{
    Tplay = ksr_get_midi_player_pos(midi_synth_ctx);
}

bool Playback::IsMidiPlayerActive()
{
    return ksr_is_midi_player_active(midi_synth_ctx);
}

bool Playback::IsMIDIEnded()
{
    return ksr_is_midi_ended(midi_synth_ctx);
}

void Playback::Close()
{
    ksr_shutdown(midi_synth_ctx);
}