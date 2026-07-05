#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "../file_helpers.h"
#include "../logger.h"
#include "../render/ui.h"
#include "soundfont_list.h"














bool                           SoundfontList::missing_files;
std::vector<std::string>       SoundfontList::missing_files_list;
std::string                    SoundfontList::soundfont_list_path;

std::vector<UI::SoundfontItem> SoundfontList::Get(std::vector<std::string> paths)
{
    std::vector<std::string>       soundfont_files;
    std::vector<UI::SoundfontItem> soundfonts;
    for(size_t i = 0; i < paths.size(); i++)
    {
        soundfont_files = FileHelpers::GetFilesByExtension(paths[i], ".sf2|.sfz|.SF2|.SFZ");
        for(size_t i = 0; i < soundfont_files.size(); ++i)
            soundfonts.push_back({ soundfont_files[i], false });
    }

    return soundfonts;
}

void SoundfontList::Save(std::vector<UI::SoundfontItem> sf_list)
{
    Log::debug("", "Saving SoundFont List: %s", soundfont_list_path.c_str());
    nlohmann::json sflist_arr = nlohmann::json::array();
    for(size_t i = 0; i < sf_list.size(); ++i)
    {
        sflist_arr.push_back({
            { "path",    sf_list[i].label   },
            { "enabled", sf_list[i].checked }
        });
    }

    std::ofstream out_file(soundfont_list_path);
    out_file << sflist_arr;
    
    if(out_file.fail())
        Log::error("", "Failed to save SoundFont list '%s', reason: %s", soundfont_list_path.c_str(), strerror(errno));
}

std::vector<UI::SoundfontItem> SoundfontList::Load()
{
    std::vector<UI::SoundfontItem> soundfonts;

    std::ifstream                  in_file(soundfont_list_path);

    if(!in_file.is_open())
        return soundfonts;

    in_file.seekg(0, std::ios::end);
    if(in_file.tellg() == 0)
    {
        Log::warn("SoundFont list file is empty!");
        return soundfonts;
    }
    in_file.seekg(0, std::ios::beg);

    nlohmann::json sflist_arr;
    in_file >> sflist_arr;
    for(size_t i = 0; i < sflist_arr.size(); ++i)
    {
        UI::SoundfontItem item;
        item.label   = sflist_arr[i]["path"].get<std::string>();
        item.checked = sflist_arr[i]["enabled"].get<bool>();
        if(std::filesystem::exists(item.label))
            soundfonts.emplace_back(item);
        else
        {
            missing_files = true;
            missing_files_list.emplace_back(item.label);
        }
    }
    return soundfonts;
}
