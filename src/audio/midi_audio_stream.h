#ifndef MIDI_AUDIO_STREAM
#define MIDI_AUDIO_STREAM
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include <kasaria.h>

class MIDIAudioStream
{
    private:
        Kasaria *ksr_ctx;
        
  public:
    SDL_AudioSpec      config;
    SDL_AudioStream   *stream;
    SDL_AudioDeviceID *devices;

    typedef struct
    {
        int         index;
        int         ID;
        std::string name;
    } Device;

    SDL_AudioDeviceID logical_audio_dev;

    static void       callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount);

    MIDIAudioStream(std::string audioDrvName, Kasaria *ksr_ctx);
    ~MIDIAudioStream();

    std::vector<std::string> driverList;
    std::vector<Device>      deviceList;

    bool                     isDiskDriverAvailable; // Todo
    bool                     isInitialized;
    void                     CreateDriverList();
    void                     CreateDeviceList();
    void                     SetAudioFilenamePath(std::string audioFName);
    void                     SetDevice(int devID_Index);
    void                     Start();
    void                     Close();
};
#endif