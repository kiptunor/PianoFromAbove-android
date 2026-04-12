#ifndef MIDI_LIST_H
#define MIDI_LIST_H


#include <string>
#include <vector>


#ifdef PLATFORM_ANDROID
    #define MIDI_LIST_FILE "/data/data/com.qsp.nvpfa/files/midis.json"
#else
    #define MIDI_LIST_FILE "midis.json"
#endif





class MidiList
{
    public:
        static std::vector<std::string> load();
        static void save(const std::vector<std::string> files, const std::string& last_midi_file);
        static bool missing_files;
        static std::vector<std::string> missing_files_list;
        static std::string midi_list_path;
        static std::string last_midi_file;
};

#endif