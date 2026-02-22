#ifndef MIDI_LIST_H
#define MIDI_LIST_H


#include <string>
#include <vector>


#ifdef PLATFORM_ANDROID
    #define MIDI_LIST_PATH "/data/data/com.qsp.nvpfa/files/midis.json"
#else
    #define MIDI_LIST_PATH "midis.json"
#endif





class MidiList
{
    public:
        static std::vector<std::string> load();
        static void save(const std::vector<std::string> files);
};

#endif