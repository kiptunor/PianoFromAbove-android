#include <algorithm>

#include <filesystem>

#include <sys/stat.h>


#include "file_helpers.h"
#include "logger.h"
#include "config/config.h"







std::string FileHelpers::config_dir;
std::string FileHelpers::lists_dir;


std::string human_readable_size(uint64_t bytes)
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);

    while(size >= 1024 && unit_index < 5)
    {
        size /= 1024;
        ++unit_index;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return oss.str();
}


std::vector<std::string> FileHelpers::GetFilesByExtension(const std::string& path, const std::string& extensions)
{
    // I'm so fucking lazy to do this shit
    std::vector<std::string> result;
        
    // Parse extensions
    std::vector<std::string> exts;
    std::string ext;
    
    // Get specified extensions separated by '|'
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
    for(const auto& entry : std::filesystem::directory_iterator(path))
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

FileHelpers::FileInfo FileHelpers::GetFileInfo(const std::string& path)
{
    FileHelpers::FileInfo res;
    
    struct stat f_stat;
    std::time_t t;
    std::tm* tm_ptr;
    
    // Get basic file information and populate the the struct members
    if(stat(path.c_str(), &f_stat) == 0)
    {
        t = f_stat.st_mtime;
        tm_ptr = std::localtime(&t);
        res.file_name = path;
        
        std::ostringstream temp;
        temp << std::put_time(tm_ptr, "D: %Y-%m-%d | T: %H:%M:%S"); // Create date & time string
        res.last_mod  = temp.str();
        res.size      = human_readable_size(f_stat.st_size);
        res.success   = true;
    }
    else
    {
        res.success = false;
        res.err << "File does not exist !!\n";
        Log::error("File to get file information: %s", res.err.str().c_str());
    }
    
    return res;
}

#ifndef PLATFORM_ANDROID
void FileHelpers::createConfigDirs()
{
    std::ostringstream config_dir_path;
    std::ostringstream lists_dir_path;
    config_dir_path << std::getenv("HOME") << CONFIG_DIR;
    lists_dir_path << std::getenv("HOME") << CONFIG_LISTS;
    
    config_dir = config_dir_path.str();
    lists_dir = lists_dir_path.str();
    
    if(!std::filesystem::exists(config_dir_path.str()))
        if(!std::filesystem::create_directory(config_dir_path.str()))
            Log::error("Failed to create config directory: %s", config_dir_path.str().c_str());
    
    
    if(!std::filesystem::exists(lists_dir_path.str()))
        if(!std::filesystem::create_directory(lists_dir_path.str()))
            Log::error("Failed to create lists directory: %s", lists_dir_path.str().c_str());
}
#endif