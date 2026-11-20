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
#include "MIDIPlayer.h"
#include "audio_enc.h"
#include "midi_audio_stream.h"








// Kasaria *midi_synth_ctx;












MIDIPlayer::MIDIPlayer()
{
    Tplay                                        = 0.0;
    is_midi_loaded                               = false;
    is_playback_started                          = false;
    playback_ended                               = false;
    is_midi_loading                              = false;
    is_midi_stream_creating                      = false;
    saved_position                               = 0;
    current_player_pos                           = 0;
    seek_amount                                  = 3.0;
    // Tscr;

    std::atomic<bool> is_midi_loaded_fn          = false;
    std::atomic<bool> is_midi_loading_fn         = false;
    std::atomic<bool> is_midi_stream_creating_fn = false;
    std::string       last_midi_path;
    // u64               playback_start_ns;

    midi_synth_ctx = ksr_init(0);

    KasariaConfig synth_cfg;

    // clang-format off
    synth_cfg =
    {
        .amplification           = 100,
        .voice_limit             = loaded_config.voice_count,
        .sample_rate             = 48000,
        .control_rate            = 100,
        .default_program         = 0,
        .low_note_velocity       = loaded_config.vel_min,
        .high_note_velocity      = loaded_config.vel_max,
        .audio_frame_size        = live_conf.audio_frame_size,
        .midi_chunk_size         = live_conf.midi_chunk_size,
        .reverb_preset           = live_conf.reverb_preset_idx,
        .immediate_panning       = false,
        .mono_audio              = false,
        .fast_decay              = true,
        .antialiasing            = true,
        .pre_resample            = true,
        .velocity_skipping       = loaded_config.vel_filter,
        .skip_initial_silence    = false,
        .allow_overlapping_notes = live_conf.overlapping_notes,
        .midi_chunk_limiter      = live_conf.midi_chunk_limiter,
        .audio_compressor        = true
    };
    // clang-format on

    ksr_set_config(midi_synth_ctx, synth_cfg);
    // ksr_print_config(midi_synth_ctx);
}

void MIDIPlayer::LoadMidi(const std::string &midi_path)
{
    if(!Midi_ctx.start_parse(midi_path.c_str()))
    {
        std::ostringstream temp_msg;
        temp_msg << "Failed to load '" << midi_path << "'";
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "MIDI Loading Error!!!!!", temp_msg.str().c_str(), nullptr);
        return;
    }

    MIDIPlayer::is_midi_loading_fn.store(true, std::memory_order_release);

    // How tf did I missed this for so long ??? :skull:
    if(live_conf.OR)
        Midi_ctx.OR();


    // - - - - [Create new MIDI stream] - - - -


    is_midi_stream_creating_fn.store(true, std::memory_order_release); // Midi stream is creating
    Log::debug("Creating MIDI Stream...");

    ksr_load_midi_file(midi_synth_ctx, MIDI_MAP, PRERENDER_MIDI_PLAYER, midi_path.c_str());
    Log::debug("MIDI Stream created.");

    is_midi_stream_creating_fn.store(false, std::memory_order_release); // Midi stream was created

    is_midi_loading_fn.store(false, std::memory_order_release);
    is_midi_loaded_fn.store(true, std::memory_order_release);
}


void MIDIPlayer::LoadDefaultSoundfonts()
{

#ifndef NON_ANDROID

    // ksr_load_soundfont_file(midi_synth_ctx, DEFAULT_GM_SOUND_FONT_PATH, true);
    // ksr_load_soundfont_file(midi_synth_ctx, DEFAULT_SOUND_FONT_PATH, true);
#else
// Todo
#endif
}

bool MIDIPlayer::LoadEnabledSoundfonts(std::vector<UI::SoundfontItem> enabled_soundfonts)
{
    bool is_enabled_sf_available = false;
    Log::debug("Loading enabled soundfonts");

    std::vector<KsrSoundfontOpts> sf_opts;

    for(size_t i = 0; i < enabled_soundfonts.size(); ++i) // Iterate through all enabled soundfonts
    {
        const auto &soundfont_item = enabled_soundfonts[i];
        if(enabled_soundfonts[i].checked)
        {
            // Set soundfont options (active_presets, bank, preset, load_percussion_bank)

            // sf_opts.emplace_back(KsrSoundfontOpts{5, 0, 0, 1});
            sf_opts.emplace_back(KsrSoundfontOpts { 128, 0, 0, 1 });

            ksr_load_soundfont_file(midi_synth_ctx, enabled_soundfonts[i].label.c_str(), sf_opts.back());
            is_enabled_sf_available = true;
        }
    }

    return is_enabled_sf_available;
}

void MIDIPlayer::ReloadSoundfonts()
{
    // Check if the midi player is active
    // Loading new soundfonts while the player is active requires the midi player to be paused first
    if(!is_paused)
    {
        // Pause the internal midi player of the midi synth
        ksr_player_pause(midi_synth_ctx);
        is_paused = true; // Also pause the midi visualizer

        // ksr_unload_soundfont(midi_synth_ctx);

        Log::debug("Stop player");

        // Load the new soundfonts
        LoadEnabledSoundfonts(live_soundfont_list);

        // Then unpause the synth's internal midi player
        ksr_player_pause(midi_synth_ctx);
        is_paused = false; // And the midi visualizer too
    }
    else // If the midi player is not active load the soundfonts as normal
        LoadEnabledSoundfonts(live_soundfont_list);
}

void MIDIPlayer::UpdateVoiceCount(int voiceCount)
{
    ksr_config_set_max_voices(midi_synth_ctx, voiceCount);

    // Update the configuration
    live_conf.voice_count = voiceCount;
}

void MIDIPlayer::LoadMidiFile(const std::string &midi_path)
{
    std::thread midi_loading_thread(&MIDIPlayer::LoadMidi, this, midi_path);


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

void MIDIPlayer::CloseMidi()
{

    ksr_player_pause(midi_synth_ctx);
    is_paused = false;

    ksr_unload_midi(midi_synth_ctx);

    // Normally the audio stream is closed once the MIDI player (kasaria midi player) reached MIDI end
    // and no audio steeam should be closed if no audio stream was initialized
    if(loaded_config.audio_drv_name == "disk" && au_stm->isInitialized)
        au_stm->Close();

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

void MIDIPlayer::UpdateState()
{
    is_midi_stream_creating = is_midi_stream_creating_fn.load();

    // This function is called every frame and the check below fires only once the midi was loaded
    if(is_midi_loaded_fn.exchange(false))
    {
        MIDIPlayer::is_midi_loaded = true;

        // Set the note velocity skipping filter everytime the midi was laoded
        ksr_config_set_note_skipping(midi_synth_ctx, live_conf.vel_min, live_conf.vel_max, live_conf.vel_filter);

        if(loaded_config.audio_drv_name == "disk")
            au_stm->SetDevice(0); // No actual audio device needed

        // Start playback
        // ksr_player_begin(midi_synth_ctx, false); // and don't wait for this to finish the entire midi
        au_stm->Start();

        Log::debug("Player started.");

        // Reset playback variables
        Tplay                    = 0.0;
        is_paused                = false;
        playback_ended           = false; // Allow the playback to start with the audio playback

        // Update current midi path
        MidiList::last_midi_file = last_midi_path;
    }
}

void MIDIPlayer::Seek_lazy(f64 seconds)
{
    if(!is_midi_loaded)
        return;

    // Get the actual playback position from the published PCM position.
    ksr_player_get_pos(midi_synth_ctx, &current_player_pos);
    f64 current_seconds = ksr_raw_pos2sec(midi_synth_ctx, current_player_pos);

    // Calculate the absolute seek target.
    f64 target_seconds  = current_seconds + seconds;
    if(target_seconds < 0.0)
        target_seconds = 0.0;

    // Seek the prerendered audio.
    ksr_player_seek(midi_synth_ctx, (long)(target_seconds * 1000.0));

    // Read the resulting playback position.
    ksr_player_get_pos(midi_synth_ctx, &current_player_pos);
    Tplay = ksr_raw_pos2sec(midi_synth_ctx, current_player_pos);

    // Reset and synchronize the visualizer.
    for(int i = 0; i < 128; ++i)
        Midi_ctx.Note_list[i].clear();

    Midi_ctx.list_seek(Tplay);
    Midi_ctx.update_to(Tplay + Tscr);

    if(playback_ended)
    {
        playback_ended = false;
        is_paused      = false;
        SDL_ResumeAudioStreamDevice(au_stm->stream);
    }
}

// prob the one above is kinda obsolete idk
void MIDIPlayer::Seek(f64 seconds)
{
    // Seek the prerendered audio.
    ksr_player_seek(midi_synth_ctx, (long)(seconds * 1000.0));

    ksr_player_get_pos(midi_synth_ctx, &current_player_pos);
    f64 current_seconds = ksr_raw_pos2sec(midi_synth_ctx, current_player_pos);

    if(playback_ended)
    {
        playback_ended = false;
        is_paused      = false;
    }

    Tplay = ksr_raw_pos2sec(midi_synth_ctx, current_player_pos);

    // Reset and synchronize the visualizer.
    for(int i = 0; i < 128; ++i)
        Midi_ctx.Note_list[i].clear();

    Midi_ctx.list_seek(Tplay);
    Midi_ctx.update_to(Tplay + Tscr);
}

void MIDIPlayer::Pause()
{
    if(!is_midi_loaded)
        return;

    MIDIPlayer::is_paused = !MIDIPlayer::is_paused;

    // Pause the internal midi player only if the MIDI was not ended
    if(!ksr_player_is_ended(midi_synth_ctx))
        ksr_player_pause(midi_synth_ctx);

    if(playback_ended)
    {
        // If at the end and we press space, restart from beginning
        ReloadSoundfonts(); // Useful for when chaning soundfonts after playback ended

        ksr_player_seek(midi_synth_ctx, 0); // restart from beginning when the midi player reaches the end

        // Set the note velocity skipping again
        ksr_config_set_note_skipping(midi_synth_ctx, live_conf.vel_min, live_conf.vel_max, live_conf.vel_filter);

        // And start the SDL audio thread again for midi playback
        SDL_ResumeAudioStreamDevice(au_stm->stream);

        Tplay          = 0.0;
        playback_ended = false;
        is_paused      = false;

        // Reset visualization state
        for(int i = 0; i < 128; ++i)
            Midi_ctx.Note_list[i].clear();

        Midi_ctx.list_seek(0);
    }
}

void MIDIPlayer::ExportAudio(std::string fname)
{
    // This sucks but it works at least

    // The prerender worker may still be running.
    if(!ksr_prerender_is_eos(midi_synth_ctx) && !ksr_player_is_ended(midi_synth_ctx))
    {
        Log::error("Player / pre-render did not finished!");
        return;
    }

    ksr_prerender_rewind(midi_synth_ctx);

    constexpr unsigned int bufferFrames = 1024;
    constexpr int          channels     = 2;

    AudioEncoder           encoder;

    if(!encoder.open(fname, ksr_config_get_sample_rate(midi_synth_ctx), channels))
        return;

    std::vector<float> buffer(bufferFrames * channels);

    for(;;)
    {
        unsigned int framesRead = 0;

        int          result     = ksr_prerender_read_buf(midi_synth_ctx, buffer.data(), bufferFrames, &framesRead);

        if(result < 0)
            break;

        // Pre-render still running and no data available yet.
        if(result == 1)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        // result == 0 and no frames = end of prerendered audio.
        if(framesRead == 0)
            break;

        if(!encoder.write(buffer.data(), framesRead))
            break;
    }

    encoder.close();
}

void MIDIPlayer::SetReverbPreset(int rv_pr_idx)
{
    ksr_config_set_reverb_preset(midi_synth_ctx, rv_pr_idx);
}

void MIDIPlayer::SetNoteVelSkipping(int low_vel, int high_vel, bool enabled)
{
    ksr_config_set_note_skipping(midi_synth_ctx, low_vel, high_vel, enabled);
}

void MIDIPlayer::SetAudioFrameSize(int frame_size)
{
    ksr_config_set_audio_frame_size(midi_synth_ctx, frame_size);
}

void MIDIPlayer::SetMidiChunkLimit(int chunk_limit, bool enabled)
{
    ksr_config_set_midi_chunk_limit(midi_synth_ctx, chunk_limit, enabled);
}

void MIDIPlayer::SetOverlappingNotes(bool enabled)
{
    ksr_config_set_overlapping_notes(midi_synth_ctx, enabled);
}

// From here these get super ez
void MIDIPlayer::UpdateEndPosition()
{
    ksr_player_get_pos(midi_synth_ctx, &current_player_pos);
    saved_position = ksr_raw_pos2sec(midi_synth_ctx, current_player_pos);
}

void MIDIPlayer::UpdateMidiPos()
{
    playback_ended = ksr_player_is_ended(midi_synth_ctx);

    ksr_player_get_pos(midi_synth_ctx, &current_player_pos);
    Tplay = ksr_raw_pos2sec(midi_synth_ctx, current_player_pos) + 0.1f; // And give just a little bit more so the piano visualizer does not remain stuck
}

bool MIDIPlayer::IsPrerenderFinished()
{
    return ksr_prerender_is_eos(midi_synth_ctx);
}

bool MIDIPlayer::IsActive()
{
    return ksr_player_is_active(midi_synth_ctx);
}

bool MIDIPlayer::IsMIDIEnded()
{
    return ksr_player_is_ended(midi_synth_ctx);
}

int MIDIPlayer::GetActiveVoices()
{
    return ksr_get_active_voices(midi_synth_ctx);
}

f64 MIDIPlayer::GetPrerenderPos()
{
    u64 pr_pos;
    ksr_prerender_get_pos(midi_synth_ctx, &pr_pos);

    return ksr_raw_pos2sec(midi_synth_ctx, pr_pos);
}

f64 MIDIPlayer::GetEndPos()
{
    u64 end_pos;
    ksr_player_get_end_pos(midi_synth_ctx, &end_pos);

    return ksr_raw_pos2sec(midi_synth_ctx, end_pos);
}

void MIDIPlayer::Close()
{
    delete au_stm;
    ksr_shutdown(midi_synth_ctx);
}

MIDIPlayer::~MIDIPlayer()
{
    ksr_shutdown(midi_synth_ctx);
}