#ifndef FILE_HELPERS_H
#define FILE_HELPERS_H

#include <sstream>
#include <string>
#include <vector>














class FileHelpers
{
  public:
    typedef struct
    {
        std::string        file_name;
        std::string        size;
        std::string        last_mod;
        bool               success; // Checks if the operation completed successfully
        std::ostringstream err;     // Insert any possible error
    } FileInfo;

    static std::vector<std::string> GetFilesByExtension(const std::string &path, const std::string &extensions);
    static FileInfo                 GetFileInfo(const std::string &path);
#ifndef PLATFORM_ANDROID
    static void        createConfigDirs();
    static std::string config_dir;
    static std::string lists_dir;
#endif
};
#endif
