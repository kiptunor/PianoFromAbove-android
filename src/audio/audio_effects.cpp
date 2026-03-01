#include <cmath>

#include <bass.h>
#include <bassmidi.h>

#include "../mb_types.h"

#include "audio_effects.h"
#include "../globals.h"



// Stollen from OmniMIDIv2
void CALLBACK dsp_limiter(u32 handle, u32 channel, void *buffer, u32 length, void *user)
{
    f32 *samples = (f32*)buffer;
    u32 count = length / sizeof(f32);
        
    for(u32 i = 0; i < count; ++i)
    {
        f32 input = samples[i];
        f32 abs_input = fabsf(input);
        
        // Envelope detection
        if (abs_input > envelope)
            envelope = limiter_attack_coeff * envelope + (1.0f - limiter_attack_coeff) * abs_input;
        else
            envelope = limiter_release_coeff * envelope + (1.0f - limiter_release_coeff) * abs_input;
        
        // Gain computation
        f32 target_gain = 1.0f;
        if (envelope > limiter_threshold)
            target_gain = (limiter_threshold + (envelope - limiter_threshold) / limiter_ratio) / envelope;
        
        // Apply gain (instant attack, smooth release)
        if (target_gain < gain)
            gain = target_gain;
        else
            gain = limiter_release_coeff * gain + (1.0f - limiter_release_coeff) * target_gain;
        
        samples[i] = input * gain * limiter_makeup_gain;
        
        //samples[i] = input * gain * limiter_makeup_gain;
    }
}


BOOL CALLBACK filter(u32 S, u32 trk, BASS_MIDI_EVENT *E, BOOL sk, void *u)
{
    if(E->event == MIDI_EVENT_NOTE)
    {
        int vel = HIBYTE(E->param);
        return vel == loaded_config.vel_min || vel > loaded_config.vel_max;
    }
    return TRUE;
}