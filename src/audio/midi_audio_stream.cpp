#include <sstream>

#include "midi_audio_stream.h"
#include "../mb_types.h"
#include "../globals.h"
#include "../logger.h"





void MIDIAudioStream::CreateDriverList()
{
    int         count   = SDL_GetNumAudioDrivers();
    const char *current = SDL_GetCurrentAudioDriver();

    for(int i = 0; i < count; ++i)
    {
        const char *driver = SDL_GetAudioDriver(i);
        driverList.emplace_back(driver);
        // std::printf("[%d] %s%s\n", i, driver ? driver : "(unknown)", current && driver && strcmp(driver, current) == 0 ? " (active)" : "");
    }
}

void MIDIAudioStream::CreateDeviceList()
{
    int count = 0;
    devices   = SDL_GetAudioPlaybackDevices(&count);

    if(!devices)
    {
        Log::error("Failed to list audio devices: %s", SDL_GetError());
        return;
    }

    for(int i = 0; i < count; ++i)
    {
        const char *name = SDL_GetAudioDeviceName(devices[i]);
        deviceList.emplace_back(Device { i, static_cast<int>(devices[i]), name });
        // std::printf("[%d] ID=%u, Name=%s\n", i, static_cast<unsigned>(devices[i]), name ? name : "(unknown)");
    }

    SDL_free(devices);
}

void MIDIAudioStream::SetDevice(int devID_Index)
{
    if(devID_Index < 0 || devID_Index >= static_cast<int>(deviceList.size()))
        return;
    SDL_AudioDeviceID id = static_cast<SDL_AudioDeviceID>(deviceList[devID_Index].ID);

    Log::trace("Device ID: %u", static_cast<unsigned>(id));
    //stream = SDL_OpenAudioDeviceStream(id, &config, callback, this); // Physical audio device is opened
    stream = SDL_OpenAudioDeviceStream(id, &config, callback, ksr_ctx);

    if(!stream)
    {
        Log::error("", "Failed to create audio stream: %s", SDL_GetError());
        return;
    }

    logical_audio_dev = SDL_GetAudioStreamDevice(stream); // Logical audio device is returned
    isInitialized     = true;

    Log::info("Audio Stream Created: %p", (void *)stream);
}

MIDIAudioStream::MIDIAudioStream(std::string audioDrvName, Kasaria *ksr_ctx) : ksr_ctx(ksr_ctx)
{
    isInitialized = false;

    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, audioDrvName.c_str());

    if(loaded_config.audio_drv_name == "disk")
        SDL_SetHint(SDL_HINT_AUDIO_DISK_TIMESCALE, "0");

    SDL_Init(SDL_INIT_AUDIO);

    config.format   = SDL_AUDIO_F32;
    config.channels = 2;
    config.freq     = DEFAULT_SAMPLE_RATE;

    std::ostringstream str;
    str << live_conf.audio_frame_size;
    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, str.str().c_str());
}

MIDIAudioStream::~MIDIAudioStream()
{
    if(loaded_config.audio_drv_name != "disk")
    {
        SDL_CloseAudioDevice(logical_audio_dev);
        SDL_DestroyAudioStream(stream);
    }
}

void MIDIAudioStream::Start()
{
    SDL_ResumeAudioStreamDevice(stream);
}

void MIDIAudioStream::Close()
{
    SDL_Delay(380); // It impacts how much time is left after the midi conversion ended
    SDL_DestroyAudioStream(stream);
    Log::info("Audio Stream Closed");
}

void     MIDIAudioStream::callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
    Kasaria *ksr_ctx_cb = (Kasaria *)userdata;
    (void)total_amount;

    if(additional_amount == 0)
        return;

    int frames = additional_amount / (int)(sizeof(f32) * 2); // stereo float
    f32 raw_audio[live_conf.audio_frame_size * 2];

    while(frames > 0)
    {
        int chunk = frames > live_conf.audio_frame_size ? live_conf.audio_frame_size : frames;

        if(ksr_ctx_cb)
            ksr_player_render(ksr_ctx_cb, raw_audio, chunk);
        else
            memset(raw_audio, 0, (size_t)chunk * 2 * sizeof(f32));

        SDL_PutAudioStreamData(stream, raw_audio, chunk * 2 * sizeof(f32));
        frames -= chunk;
    }
}