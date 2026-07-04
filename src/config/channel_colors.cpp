#include "channel_colors.h"
#include "../logger.h"
#include "config.h"
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>














bool          ChannelColors::more_channel_colors = false;

unsigned int *ChannelColors::getChannelColors(std::string filename)
{
    if(!std::filesystem::exists(filename))
    {
        Log::error("", "Failed to get channel colors presets File '%s' was not found !!", filename.c_str());
        return nullptr;
    }

    std::ifstream file(filename);
    if(!file.is_open())
    {
        Log::error("", "Failed to get channel colors presets from %s", filename.c_str());
        return nullptr;
    }

    std::string line;
    std::string content;
    bool        inMultiLineComment = false;

    while(std::getline(file, line))
    {
        if(inMultiLineComment)
        {
            size_t end = line.find("*/");
            if(end != std::string::npos)
            {
                inMultiLineComment = false;
                line               = line.substr(end + 2);
            }
            else
                continue;
        }

        size_t commentStart = line.find("//");
        if(commentStart != std::string::npos)
            line = line.substr(0, commentStart);

        size_t multiStart = line.find("/*");
        if(multiStart != std::string::npos)
        {
            size_t multiEnd = line.find("*/", multiStart + 2);
            if(multiEnd != std::string::npos)
                line.erase(multiStart, multiEnd - multiStart + 2);
            else
            {
                line               = line.substr(0, multiStart);
                inMultiLineComment = true;
            }
        }

        size_t firstChar = line.find_first_not_of(" \t");
        if(firstChar != std::string::npos)
        {
            size_t lastChar  = line.find_last_not_of(" \t");
            content         += line.substr(firstChar, lastChar - firstChar + 1);
        }
    }

    // Remove any remaining whitespace from the end of the string
    size_t lastIndex = content.length();
    while(lastIndex > 0 && content[lastIndex - 1] == ' ')
        lastIndex--;

    // Now Read the string and look for each channel color
    unsigned int               *ch_colors = new unsigned int[16];
    std::regex                  colorRegex(R"(#[a-fA-F0-9]{6})");
    std::smatch                 match;
    std::string::const_iterator searchStart = content.cbegin();
    int                         count       = 0;

    while(std::regex_search(searchStart, content.cend(), match, colorRegex))
    {
        if(count > 15)
            more_channel_colors = true;
        else
            ch_colors[count] = Config::hexToUInt(match[0].str());

        searchStart = match.suffix().first;
        count++;
    }

    if(more_channel_colors)
        Log::warn("Dear user! MIDIs have 16 channels so then why you're trying to push more colors for no reason ?");

    return ch_colors;
}
