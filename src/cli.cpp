


#include "cli.h"





// Define optional arguments with defaults
#define OPTIONAL_ARGS \
    OPTIONAL_INT_ARG(fps, 1, "-fps", "Set custom FPS") \
    OPTIONAL_INT_ARG(voice_count, 1, "-vc", "Set custom voice count") \
    OPTIONAL_STRING_ARG(midi_file, "mf", "Midi input file path") \
    OPTIONAL_STRING_ARG(soundfont_file, "sf", "Soundfont file path")

// Define boolean flags
#define BOOLEAN_ARGS \
    BOOLEAN_ARG(help, "-h", "Show help") \
    BOOLEAN_ARG(vsync, "-vsync", "Enable vertical synchronization") \
    BOOLEAN_ARG(ignore_config_file, "-icf", "Ignore config file (Default settings will be used)")

#include <easyargs.h>


void CLI::parseArgs(int ac, char** av)
{
    args_t args = make_default_args();
    
    if(!parse_args(ac, av, &args) || args.help)
    {
        print_help(av[0]);
        //return 1;
    }
}