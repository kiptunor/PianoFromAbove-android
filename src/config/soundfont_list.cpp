#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "soundfont_list.h"
#include "../file_helpers.h"
#include "../render/ui.h"




std::vector<std::string> SoundfontList::GetSoundFontFiles()
{
    std::vector<std::string> soundfont_files;
    soundfont_files = FileHelpers::GetFilesByExtension("/home/andre/disks/1_TB_1/bm/soundfonts", ".sf2|.sfz");
    return soundfont_files;
}

std::vector<UI::SoundfontItem> SoundfontList::Get()
{
    std::vector<UI::SoundfontItem> soundfonts;
    std::vector<std::string> files = GetSoundFontFiles();
    for(size_t i = 0; i < files.size(); ++i)
        soundfonts.push_back({files[i], false});
    
    return soundfonts;
}

void SoundfontList::Save(std::vector<UI::SoundfontItem> sf_list)
{
    nlohmann::json sflist_arr = nlohmann::json::array();
    for(size_t i = 0; i < sf_list.size(); ++i)
    {
        sflist_arr.push_back({
            {"path", sf_list[i].label},
            {"enabled", sf_list[i].checked}
        });
    }
    std::ofstream out_file(SOUNDFONT_LIST_PATH);
    out_file << sflist_arr;
}

std::vector<UI::SoundfontItem> SoundfontList::Load()
{
    std::vector<UI::SoundfontItem> soundfonts;
    std::ifstream in_file(SOUNDFONT_LIST_PATH);
    if(!in_file.is_open())
        return soundfonts;

    nlohmann::json sflist_arr;
    in_file >> sflist_arr;
    for(size_t i = 0; i < sflist_arr.size(); ++i)
    {
        UI::SoundfontItem item;
        item.label = sflist_arr[i]["path"].get<std::string>();
        item.checked = sflist_arr[i]["enabled"].get<bool>();
        soundfonts.push_back(item);
    }
    return soundfonts;
}

void SoundfontList::Refresh()
{
    std::vector<std::string> files = GetSoundFontFiles();
}