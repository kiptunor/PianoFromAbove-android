#ifndef CLI_H
#define CLI_H


#include <string>



class CLI
{
    public:
        typedef struct
        {
            int fps;
            int voice_count;
            bool vsync;
            bool ignore_config_file;
            std::string midi_file;
            std::string soundfont_file;
        }options;
        static void parseArgs(int ac, char** av);
        options getOptions();
};
#endif