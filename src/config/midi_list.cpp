#include <nlohmann/json.hpp>
#include <fstream>



#include "midi_list.h"
#include "../logger.h"




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
    
    return midi_list_arr["PreviousMidiFiles"].get<std::vector<std::string>>();
}