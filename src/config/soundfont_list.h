#ifndef SOUNDFONT_LIST_H
#define SOUNDFONT_LIST_H



#include "../render/ui.h"


#ifdef PLATFORM_ANDROID
    #define SOUNDFONT_LIST_PATH "/data/data/com.qsp.nvpfa/files/soundfonts.json"
#else
    #define SOUNDFONT_LIST_PATH "soundfonts.json"
#endif


class SoundfontList
{
    public:
        //static std::vector<std::string> GetSoundFontFiles();
        static void Save(std::vector<UI::SoundfontItem> sf_list);
        static std::vector<UI::SoundfontItem> Get(std::vector<std::string> paths);
        static std::vector<UI::SoundfontItem> Load();
        //static void Refresh();
        static void Clear();
        static bool missing_files;
        static std::vector<std::string> missing_files_list;
};
#endif