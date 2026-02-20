#ifndef FILE_HELPERS_H
#define FILE_HELPERS_H


#include <string>
#include <vector>



class FileHelpers
{
    public:
        static std::vector<std::string> GetFilesByExtension(const std::string& path, const std::string& extensions);
};
#endif