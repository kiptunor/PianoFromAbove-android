#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>



#include "midi_list.h"
#include "../logger.h"




bool MidiList::missing_files;
std::vector<std::string> MidiList::missing_files_list;


void MidiList::save(const std::vector<std::string> files)
{
    nlohmann::json midi_list_arr = nlohmann::json::object();
    midi_list_arr["PreviousMidiFiles"] = files;
    
    std::ofstream file(MIDI_LIST_PATH);
    file << midi_list_arr.dump(4);
}

std::vector<std::string> MidiList::load()
{
    std::ifstream file(MIDI_LIST_PATH);
    if(!file.is_open())
    {
        Log::error("Failed to open MIDI list file");
        return {};
    }
    
    nlohmann::json midi_list_arr;
    file >> midi_list_arr;
    
    if(!midi_list_arr.contains("PreviousMidiFiles"))
    {
        Log::error("MIDI list array not found");
        return {};
    }
    
    std::vector<std::string> midi_files;
    
    for(const auto& file : midi_list_arr["PreviousMidiFiles"])
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