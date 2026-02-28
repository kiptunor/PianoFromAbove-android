#ifndef FILE_HELPERS_H
#define FILE_HELPERS_H


#include <string>
#include <vector>
#include <sstream>



class FileHelpers
{
    public:
        typedef struct
        {
            std::string file_name;
            std::string size;
            std::string last_mod;
            bool success;           // Checks if the operation completed successfully
            std::ostringstream err; // Insert any possible error
        }FileInfo;
        
        
        static std::vector<std::string> GetFilesByExtension(const std::string& path, const std::string& extensions);
        static FileInfo GetFileInfo(const std::string& path);
};
#endif