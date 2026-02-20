#ifndef AUDIO_EFFECTS_H
#define AUDIO_EFFECTS_H


#include <bass.h>
#include <bassmidi.h>

#include "../mb_types.h"


// Limiter settings
inline float limiter_threshold = 0.8f;
inline float limiter_knee = 0.05f;
inline float limiter_attack = 0.1f;     // attack speed (0 = instant, 1 = never)
inline float limiter_release = 0.0005f; // release speed
inline float current_gain = 1.0f;



// Audio limiter
void CALLBACK dsp_limiter(u32 handle, u32 channel, void *buffer, u32 length, void *user);

// Ignore notes with specific velocity
BOOL CALLBACK filter(u32 S, u32 trk, BASS_MIDI_EVENT *E, BOOL sk, void *u);
#endif