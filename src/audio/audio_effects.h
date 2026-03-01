#ifndef AUDIO_EFFECTS_H
#define AUDIO_EFFECTS_H


#include <math.h>

#include <bass.h>
#include <bassmidi.h>

#include "../mb_types.h"


// Limiter settings
inline f32 limiter_threshold = 0.3f;
inline f32 limiter_ratio = 1000.0f;  // high ratio = limiter
inline f32 limiter_attack_ms = 10.0f;
inline f32 limiter_release_ms = 50.0f;
inline f32 limiter_sample_rate = 44100.0f;
inline f32 limiter_attack_coeff = expf(-1.0f / (limiter_attack_ms * 0.001f * limiter_sample_rate));
inline f32 limiter_release_coeff = expf(-1.0f / (limiter_release_ms * 0.001f * limiter_sample_rate));
inline f32 envelope = 0.0f;
inline f32 gain = 1.0f;
inline f32 limiter_makeup_gain = 1.2f;



// Audio limiter
void CALLBACK dsp_limiter(u32 handle, u32 channel, void *buffer, u32 length, void *user);

// Ignore notes with specific velocity
BOOL CALLBACK filter(u32 S, u32 trk, BASS_MIDI_EVENT *E, BOOL sk, void *u);
#endif