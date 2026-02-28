#include <fstream>
#include <nlohmann/json.hpp>

#include "soundfont_list.h"
#include "../file_helpers.h"
#include "../render/ui.h"




std::vector<UI::SoundfontItem> SoundfontList::Get(std::vector<std::string> paths)
{
    std::vector<std::string> soundfont_files;
    std::vector<UI::SoundfontItem> soundfonts;
    for(size_t i = 0; i < paths.size(); i++)
    {
        soundfont_files = FileHelpers::GetFilesByExtension(paths[i], ".sf2|.sfz");
        for(size_t i = 0; i < soundfont_files.size(); ++i)
            soundfonts.push_back({soundfont_files[i], false});
    }
    
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