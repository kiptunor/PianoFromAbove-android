#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>



#include "../logger.h"
#include "midi_list.h"














bool                     MidiList::missing_files;
std::vector<std::string> MidiList::missing_files_list;
std::string              MidiList::midi_list_path;
std::string              MidiList::last_midi_file;


void                     MidiList::save(const std::vector<std::string> files, const std::string &last_midi_file)
{
    nlohmann::json midi_list_arr       = nlohmann::json::object();
    midi_list_arr["lastMidiFile"]      = last_midi_file;
    midi_list_arr["PreviousMidiFiles"] = files;

    
    //Log::debug("", "Saving MIDI List: %s", midi_list_path.c_str());

    std::ofstream file(midi_list_path);

    file << midi_list_arr.dump(4);
    
    if(file.fail())
        Log::error("", "Failed to save midi list '%s', reason: %s", midi_list_path.c_str(), strerror(errno));
}

std::vector<std::string> MidiList::load()
{
    std::ifstream            file(midi_list_path);
    std::vector<std::string> midi_files;

    if(!file.is_open() || !file.good())
    {
        Log::error("Failed to open MIDI list file");
        return midi_files;
    }

    file.seekg(0, std::ios::end);
    if(file.tellg() == 0)
    {
        Log::warn("MIDI list file is empty!");
        return midi_files;
    }
    file.seekg(0, std::ios::beg);

    nlohmann::json midi_list_arr;
    file >> midi_list_arr;

    last_midi_file = midi_list_arr.value("lastMidiFile", "");

    if(!midi_list_arr.contains("PreviousMidiFiles"))
    {
        Log::error("MIDI list array not found");
        return midi_files;
    }

    for(const auto &file : midi_list_arr["PreviousMidiFiles"])
    {
        if(std::filesystem::exists(file))
            midi_files.emplace_back(file.get<std::string>());
        else
        {
            missing_files = true;
            missing_files_list.emplace_back(file.get<std::string>());
        }
    }

    return midi_files;
}
