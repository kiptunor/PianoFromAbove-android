/*

    Kasaria -- A powerful and High efficiency MIDI Synth based on TiMidity
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>
    Copyright (C) 2026 Kiptunor

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


   timid.h
*/

#ifndef KASARIA_H
#define KASARIA_H

#include <stdbool.h>
#include <stdint.h>



#if defined(_WIN32)
    #if defined(__TINYC__)
        #define __declspec(x) __attribute__((x))
    #endif
    #if defined(BUILD_LIBTYPE_SHARED)
        #define KSR_API __declspec(dllexport) // Win32 DLL API Export
    #elif defined(USE_LIBTYPE_SHARED)
        #define KSR_API __declspec(dllimport) // Win32 DLL API Import
    #endif
#else
    #if defined(BUILD_LIBTYPE_SHARED)
        #define KSR_API __attribute__((visibility("default"))) // Unix Shared Library API Export
    #endif
#endif

#ifndef KSR_API
    #define KSR_API // Functions defined as 'extern' by default (implicit specifiers)
#endif

// clang-format off
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct Kasaria Kasaria;

typedef struct
{
    signed char data[3];
}int24;


typedef struct
{
    int amplification;
    int voice_limit;
    int sample_rate;
    int control_rate;
    int default_program;
    int low_note_velocity;
    int high_note_velocity;
    int audio_frame_size;
    bool immediate_panning;
    bool mono_audio;
    bool fast_decay;
    bool antialiasing;
    bool pre_resample;
    bool velocity_skipping;
    bool skip_initial_silence;
    bool audio_compressor;
}KasariaConfig;

/*
    General API notes: Unless otherwise indicated, functions that return a value will return non-0 on success,
    and 0 on failure. Time and duration is represented in milliseconds. API functions for getting strings return
    the length of the string, regardless if a null string pointer is passed or not
*/

#define LOG_LEVEL_FATAL  0
#define LOG_LEVEL_ERROR  1
#define LOG_LEVEL_WARN   2
#define LOG_LEVEL_INFO   3
#define LOG_LEVEL_DEBUG  4
#define LOG_LEVEL_TRACE  5


// Soon to be added as argument to ksr_play_midi
#define REALTIME_MIDI_PLAYER  11
#define PRERENDER_MIDI_PLAYER 12

// Audio format identifiers for ksr_play_midi
#define AUDIO_CHAR   21
#define AUDIO_SHORT  22
#define AUDIO_INT24  23
#define AUDIO_LONG   24
#define AUDIO_FLOAT  25 // Standard audio format (Greater software compatibility)
#define AUDIO_DOUBLE 26
#define AUDIO_ULAW   27

// MIDI Loading modes
#define MIDI_MEMORY  31 // Load MIDI data into memory
#define MIDI_MAP     32 // Play MIDI File by reading it from disk (No RAM is used)

// Audio initialization scope
#define INTERNAL_MIDI_PLAYER 41
#define RAW_MIDI_EVENTS      42

// Allocate and initialize an instance of Kasaria
KSR_API Kasaria *ksr_init(bool disable_logs);
KSR_API void ksr_set_log_level(Kasaria *ksr, int level);
KSR_API int ksr_init_audio(Kasaria *ksr, int init_scope);
KSR_API int ksr_start_audio(Kasaria *ksr);
KSR_API int ksr_stop_audio(Kasaria *ksr);

KSR_API void ksr_print_config(Kasaria *ksr);                     // Print the configuration to stdout at any time
KSR_API KasariaConfig ksr_get_config(Kasaria *ksr);              // Read the current config at any time
KSR_API void ksr_set_config(Kasaria *ksr, KasariaConfig config); // Set config using the provided struct (This overrides majority of the settings)

KSR_API void ksr_restore_defaults(Kasaria *ksr); // Restore default settings



// --------------------------- Settings API ---------------------------
KSR_API void ksr_set_amplification(Kasaria *ksr, int amplification); // Amplification is represented in percent
KSR_API void ksr_set_max_voices(Kasaria *ksr, int voices);           // The number of voices is clamped between 1 and MAX_VOICES
KSR_API void ksr_set_immediate_panning(Kasaria *ksr, bool value);    // The value argument for the following functions should be treated as a boolean
KSR_API void ksr_set_mono(Kasaria *ksr, bool value); //This makes weird noise // Renders mono audio buffers if enabled, interleaved stereo otherwise

// These next few functions reload the current sample bank before returning
KSR_API void ksr_set_fast_decay(Kasaria *ksr, bool value);
KSR_API void ksr_set_antialiasing(Kasaria *ksr, bool value);
KSR_API void ksr_set_pre_resample(Kasaria *ksr, bool value);
KSR_API void ksr_set_sample_rate(Kasaria *ksr, int rate);                   // The sample rate is clamped between MIN_OUTPUT_RATE and MAX_OUTPUT_RATE
KSR_API void ksr_set_control_rate(Kasaria *ksr, int rate);                  // The control rate is clamped between current sample rate / MAX_CONTROL_RATIO and current sample rate
KSR_API void ksr_set_default_program(Kasaria *ksr, int program);            // Sets the default MIDI program, takes effect on next MIDI reset
KSR_API void ksr_set_drum_channel(Kasaria *ksr, int channel, bool enable);
KSR_API void ksr_set_quiet_channel(Kasaria *ksr, int channel, bool enable);
KSR_API void ksr_set_note_velocity_skipping(Kasaria *ksr, uint8_t low_vel, uint8_t high_vel, bool enabled);
KSR_API void ksr_set_audio_compressor(Kasaria *ksr, bool enabled);
KSR_API void ksr_set_audio_frame_size(Kasaria *ksr, int size);


// --------------------------- Parameters Reading API ---------------------------
KSR_API int ksr_get_amplification(Kasaria *ksr);
KSR_API int ksr_get_active_voices(Kasaria *ksr);
KSR_API int ksr_get_max_voices(Kasaria *ksr);
KSR_API int ksr_get_immediate_panning(Kasaria *ksr);
KSR_API int ksr_get_mono(Kasaria *ksr);
KSR_API int ksr_get_fast_decay(Kasaria *ksr);
KSR_API int ksr_get_antialiasing(Kasaria *ksr);
KSR_API int ksr_get_pre_resample(Kasaria *ksr);
// KSR_API int ksr_get_dynamic_instrument_load(Kasaria *ksr); // Unused and idk what to do with this
KSR_API int ksr_get_sample_rate(Kasaria *ksr);
KSR_API int ksr_get_control_rate(Kasaria *ksr);
KSR_API int ksr_get_default_program(Kasaria *ksr);
KSR_API int ksr_get_drum_channel_enabled(Kasaria *ksr, int channel);
KSR_API int ksr_get_quiet_channel_enabled(Kasaria *ksr, int channel);
KSR_API int ksr_get_lost_notes(Kasaria *ksr);
KSR_API int ksr_get_cut_notes(Kasaria *ksr);

// Get values from a given MIDI channel
KSR_API int ksr_channel_get_volume(Kasaria *ksr, int channel);
KSR_API int ksr_channel_get_pan(Kasaria *ksr, int channel);
KSR_API int ksr_channel_get_expression(Kasaria *ksr, int channel);
KSR_API int ksr_channel_get_sustain(Kasaria *ksr, int channel);
KSR_API int ksr_channel_get_pitch_wheel(Kasaria *ksr, int channel);
KSR_API int ksr_channel_get_pitch_range(Kasaria *ksr, int channel);
KSR_API int ksr_channel_get_program(Kasaria *ksr, int channel);
// The following function returns -1 for drum channels
KSR_API int ksr_channel_get_bank(Kasaria *ksr, int channel);
KSR_API int ksr_channel_get_mono(Kasaria *ksr, int channel);

// These are for the MIDI file player
KSR_API int ksr_get_smf_name(Kasaria *ksr, char *buffer, long count);
KSR_API int ksr_get_event_count(Kasaria *ksr);
KSR_API int ksr_get_sample_count(Kasaria *ksr);
KSR_API int ksr_get_current_sample_position(Kasaria *ksr);
KSR_API int ksr_get_duration(Kasaria *ksr);
KSR_API int ksr_get_current_time(Kasaria *ksr);
// Bitrate is measured in KBPS
KSR_API int ksr_get_bitrate(Kasaria *ksr);
KSR_API int ksr_get_song_title(Kasaria *ksr, char *buffer, long count);
KSR_API int ksr_get_song_copyright(Kasaria *ksr, char *buffer, long count);


KSR_API int  ksr_load_soundfont_file(Kasaria *ksr, const char *filename, bool preload_instruments); // Currently supports only SF2 format (SFZ Support is also planned)
KSR_API void ksr_load_soundfont_from_mem(Kasaria *ksr, void *mem, long size, bool preload_instruments); // Todo


// --------------------------- MIDI Player API ---------------------------
KSR_API int  ksr_load_midi_file(Kasaria *ksr, int loading_mode, const char *filename); // MIDI file player, only supports standard MIDI files
KSR_API int  ksr_load_midi_from_mem(Kasaria *ksr, void *mem, long size); // Todo
KSR_API short ksr_get_midi_file_format(Kasaria *ksr);
KSR_API int  ksr_get_loaded_midi_track(Kasaria *ksr);
KSR_API int  ksr_get_midi_track_count(Kasaria *ksr);
KSR_API void ksr_skip_initial_silence(Kasaria *ksr, bool is);
KSR_API void ksr_unload_midi(Kasaria *ksr);
KSR_API int  ksr_reload_midi(Kasaria *ksr);
KSR_API int  ksr_play_midi_raw(Kasaria *ksr, long type, unsigned char *buffer, long count); // count is in samples
KSR_API int  ksr_play_midi(Kasaria *ksr, bool wait_midi_ending);                       // Play MIDI asynchronously (Audio streaming is handeled internally)
KSR_API int  ksr_seek_midi(Kasaria *ksr, long time);                                    // Absolute seeking
KSR_API int  ksr_fast_forward_midi(Kasaria *ksr, long time);                            // Relative seeking
KSR_API bool ksr_is_midi_player_active(Kasaria *ksr);
KSR_API bool ksr_pause_midi(Kasaria *ksr);
KSR_API int  ksr_rewind_midi(Kasaria *ksr, long time);
KSR_API int  ksr_restart_midi(Kasaria *ksr); // Unused
KSR_API bool ksr_is_midi_ended(Kasaria *ksr);
KSR_API int  ksr_stop_midi(Kasaria *ksr);
KSR_API double ksr_get_midi_player_pos(Kasaria *ksr);

// --------------------------- MIDI Event API ---------------------------
KSR_API void ksr_channel_note_on(Kasaria *ksr, unsigned char channel, unsigned char note, unsigned char velocity);
KSR_API void ksr_channel_note_off(Kasaria *ksr, unsigned char channel, unsigned char note);
KSR_API void ksr_channel_key_pressure(Kasaria *ksr, unsigned char channel, unsigned char note, unsigned char velocity);
KSR_API void ksr_channel_set_volume(Kasaria *ksr, unsigned char channel, unsigned char volume);
KSR_API void ksr_channel_set_pan(Kasaria *ksr, unsigned char channel, unsigned char pan);
KSR_API void ksr_channel_set_expression(Kasaria *ksr, unsigned char channel, unsigned char expression);
KSR_API void ksr_channel_set_sustain(Kasaria *ksr, unsigned char channel, unsigned char sustain);
KSR_API void ksr_channel_set_pitch_wheel(Kasaria *ksr, unsigned char channel, unsigned short pitch);
KSR_API void ksr_channel_set_pitch_range(Kasaria *ksr, unsigned char channel, unsigned char range);
KSR_API void ksr_channel_set_program(Kasaria *ksr, unsigned char channel, unsigned char program);
KSR_API void ksr_channel_set_bank(Kasaria *ksr, unsigned char channel, unsigned char bank);
KSR_API void ksr_channel_mono_mode(Kasaria *ksr, unsigned char channel);
KSR_API void ksr_channel_poly_mode(Kasaria *ksr, unsigned char channel);
KSR_API void ksr_channel_all_notes_off(Kasaria *ksr, unsigned char channel);
KSR_API void ksr_channel_all_sounds_off(Kasaria *ksr, unsigned char channel);
KSR_API void ksr_channel_reset_controllers(Kasaria *ksr, unsigned char channel);
KSR_API void ksr_channel_control_change(Kasaria *ksr, unsigned char channel, unsigned char controller, unsigned char value);
KSR_API void ksr_all_notes_off(Kasaria *ksr);     // Stop all notes with release
KSR_API void ksr_all_sounds_off(Kasaria *ksr);    // Stop all notes with quick fade out, helps avoid clicks
KSR_API void ksr_reset_controllers(Kasaria *ksr); // Reset all MIDI controllers
KSR_API void ksr_panic(Kasaria *ksr);             // Stop all notes immediately
KSR_API void ksr_reset(Kasaria *ksr);             // Stop all notes immediately, and reset all MIDI parameters

// Low level input API
KSR_API void ksr_write_midi_ev(Kasaria *ksr, unsigned char byte1, unsigned char byte2, unsigned char byte3); // Requires long MIDI data manipulation
KSR_API void ksr_write_midi_ev_packed(Kasaria *ksr, unsigned long data);                                     // Send short MIDI events
KSR_API void ksr_write_sysex(Kasaria *ksr, unsigned char *buffer, long count);

// Audio output functions
KSR_API void ksr_render_char(Kasaria *ksr, unsigned char *buffer, long count);
KSR_API void ksr_render_short(Kasaria *ksr, short *buffer, long count);
KSR_API void ksr_render_int24(Kasaria *ksr, int24 *buffer, long count);
KSR_API void ksr_render_long(Kasaria *ksr, long *buffer, long count);
KSR_API void ksr_render_float(Kasaria *ksr, float *buffer, long count);
KSR_API void ksr_render_double(Kasaria *ksr, double *buffer, long count);
KSR_API void ksr_render_ulaw(Kasaria *ksr, unsigned char *buffer, long count);




// --------------------------- Utility functions ---------------------------
KSR_API int ksr_millis2samples(Kasaria *ksr, long millis);
KSR_API int ksr_samples2millis(Kasaria *ksr, long samples);

// Close and free an instance of Kasaria. This should be called after all other API function calls
KSR_API void ksr_shutdown(Kasaria *ksr);

#ifdef __cplusplus
}
#endif

#endif