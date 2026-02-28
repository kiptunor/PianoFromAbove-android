#include <cmath>

#include <bass.h>
#include <bassmidi.h>

#include "../mb_types.h"

#include "audio_effects.h"
#include "../globals.h"




void CALLBACK dsp_limiter(u32 handle, u32 channel, void *buffer, u32 length, void *user)
{
    float *samples = (float*)buffer;
    u32 count = length / sizeof(float);
   
    for(u32 i = 0; i < count; ++i)
    {
        float input = samples[i];
        float abs_input = fabs(input);
   
        float desired_gain = 1.0f;
   
        if(abs_input > limiter_threshold)
        {
            float exceed = abs_input - limiter_threshold;
            float compressed = exceed / (exceed + limiter_knee);
            desired_gain = limiter_threshold / (limiter_threshold + compressed);
        }
   
        // Smoothly approach desired gain
        if(desired_gain < current_gain)
            current_gain += (desired_gain - current_gain) * limiter_attack; // attack smoothing
        else
            current_gain += (desired_gain - current_gain) * limiter_release; // release smoothing
   
        samples[i] *= current_gain;
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