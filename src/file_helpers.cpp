#include <algorithm>

#include <filesystem>


#include "file_helpers.h"


// Don't forget to link -lstdc++fs

std::vector<std::string> FileHelpers::GetFilesByExtension(const std::string& path, const std::string& extensions)
{
    // I'm so fucking lazy to do this shit
    std::vector<std::string> result;
        
    // Parse extensions
    std::vector<std::string> exts;
    std::string ext;
    for(char c : extensions)
    {
        if(c == '|')
        {
            if(!ext.empty())
                exts.push_back(ext);
            
            ext.clear();
        }
        else
        {
            ext += c;
        }
    }
    if(!ext.empty())
        exts.push_back(ext);
        
        // Convert to lowercase
    for(auto& e : exts)
    {
        std::transform(e.begin(), e.end(), e.begin(), ::tolower);
    }
        
    // Scan directory
    for(const auto& entry : std::filesystem::recursive_directory_iterator(path))
    {
        if(entry.is_regular_file())
        {
            std::string fileExt = entry.path().extension().string();
            std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::tolower);
                
            for(const auto& e : exts)
            {
                if(fileExt == e)
                {
                    result.push_back(entry.path().string());
                    break;
                }
            }
        }
    }
        
    return result;
}