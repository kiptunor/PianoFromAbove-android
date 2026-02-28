#ifndef CLI_H
#define CLI_H


#include <string>


/*
    This could be a nice hidden feature for android users xd
*/



class CLI
{
    public:
        
        static int fps;
        static int voice_count;
        static bool vsync;
        static bool ignore_config_file;
        static bool no_text_dbg;
        static std::string midi_file;
        static std::string soundfont_file;
        
        static void parseArgs(int ac, char** av);
};
#endif