#include <iostream>
#include <clipp.h>


#include "cli.h"






int CLI::fps;
int CLI::voice_count;
bool CLI::vsync;
bool CLI::ignore_config_file;
bool CLI::no_text_dbg;
std::string CLI::midi_file;
std::string CLI::soundfont_file;


void CLI::parseArgs(int ac, char** av)
{
    auto cli = (
        clipp::option("-imf", "--input-midi-file").set(midi_file).doc("Specify a midi file"),
        clipp::option("-isf", "--input-soundfont-file").set(soundfont_file).doc("Specify a soundfont file file (.sf2 / .sfz)"),
        clipp::option("-fps").set(fps).doc("Set FPS"),
        clipp::option("-vsync").set(vsync).doc("Enable vertical sync"),
        clipp::option("-vc", "--voice-count").set(voice_count).doc("Set voice count"),
        clipp::option("-icf", "--ignore-config-file").set(ignore_config_file).doc("Ignore config file"),
        clipp::option("-ntd", "--no-text-debug").set(no_text_dbg).doc("Avoid debug logging to text file")
    );
    
    clipp::parse(ac, av, cli);
    std::cout << clipp::make_man_page(cli, av[0]);
}