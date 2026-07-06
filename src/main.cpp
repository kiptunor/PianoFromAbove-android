#include <filesystem>


#include <SDL3/SDL.h>
#include <backend_render/imgui_impl_sdl3.h>
#include <bass.h>
#include <bassmidi.h>


#if defined(_WIN32) || defined(_WIN64)
    #define BASSMIDI_LIB "bassmidi.dll"
#else
    #define BASSMIDI_LIB "libbassmidi.so"
#endif



#ifdef PLATFORM_ANDROID
    #define APP_ENTRY SDL_main
#else
    #define APP_ENTRY main
#endif


// #define DEBUG

#include "audio/playback.h"
#include "config/config.h"
#include "config/file_dialog_state.h"
#include "config/midi_list.h"
#include "config/soundfont_list.h"
#include "file_helpers.h"
#include "globals.h"
#include "logger.h"
#include "visualizer_handler.h"
#ifndef PLATFORM_ANDROID
    #include "cli.h"
#endif














NVnoteList         Midi_ctx;
VisualizerHandler *vis;





void               AudioSetup()
{
    BASS_PluginLoad(BASSMIDI_LIB, 0);
    BASS_SetConfig(BASS_CONFIG_BUFFER, 5000);
    BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 1); // The higher the value the longer the stutters / audio underruns
    BASS_SetConfig(BASS_CONFIG_MIDI_AUTOFONT, 0);
}




extern "C" int APP_ENTRY(int argc, char *argv[])
{
    // CLI Parsing
    //if(argc > 1)
    //    CLI::parseArgs(argc, argv); // The same CLI should also work on Android

    // Log::createFile("nv_pfa.log");

#ifndef PLATFORM_ANDROID
    FileHelpers::createConfigDirs();
    std::ostringstream config_path;
    config_path << FileHelpers::config_dir << "/" << CONFIG_FILE;
    Config::config_path = config_path.str();

    std::ostringstream midi_list_path;
    midi_list_path << FileHelpers::lists_dir << "/" << MIDI_LIST_FILE;
    MidiList::midi_list_path = midi_list_path.str();

    std::ostringstream soundfont_list_path;
    soundfont_list_path << FileHelpers::lists_dir << "/" << SOUNDFONT_LIST_FILE;
    SoundfontList::soundfont_list_path = soundfont_list_path.str();

    std::ostringstream file_dialog_state_path;
    file_dialog_state_path << FileHelpers::lists_dir << "/" << FD_STATE_FILE_PATH;
    FileDialogState::json_file_path = file_dialog_state_path.str();
#else
    std::ostringstream config_path;
    config_path << CONFIG_FILE;

    std::ostringstream midi_list_path;
    midi_list_path << MIDI_LIST_FILE;

    std::ostringstream soundfont_list_path;
    soundfont_list_path << SOUNDFONT_LIST_FILE;

    std::ostringstream file_dialog_state_path;
    file_dialog_state_path << FD_STATE_FILE_PATH;
    
    Config::config_path = config_path.str();
    MidiList::midi_list_path = midi_list_path.str();
    SoundfontList::soundfont_list_path = soundfont_list_path.str();
#endif

    /*
    - - - - Configuration Handling - - - -
    */
    if(!std::filesystem::exists(config_path.str()))
        live_conf = default_settings;
    else
    {
        loaded_config = Config::Load();
        live_conf     = loaded_config;
    }

    Log::log_to_internal_buf = live_conf.internal_log_buffer;

    if(live_conf.log_to_file)
        Log::createFile("nv_pfa.log");

    /* - - - - Soundfont List Handling - - - - */
    if(std::filesystem::exists(soundfont_list_path.str()))
        loaded_soundfont_list = SoundfontList::Load();


    /* - - - - MIDI List Handling - - - - */
    if(std::filesystem::exists(midi_list_path.str()))
    {
        loaded_midi_list = MidiList::load();
        live_midi_list   = loaded_midi_list;
    }

    
    /* - - - - File Dialog State Handling - - - - */
    live_fd_state = FileDialogState::load();

    if(!std::filesystem::exists(DEFAULT_SOUND_FONT_PATH))
        Log::warn("", "Default soundfont not found: %s", DEFAULT_SOUND_FONT_PATH);

    if(!std::filesystem::exists(DEFAULT_GM_SOUND_FONT_PATH))
        Log::warn("", "Default GM soundfont not found: %s", DEFAULT_GM_SOUND_FONT_PATH);


    // Get available audio devices
    // Required for when the user wants to change the audio device
    availableAudioDevices = Playback::GetAudioOutputs();

    AudioSetup();

    if(availableAudioDevices.size() == 0)
    {
        Log::warn("No audio devices found!!!");
        Log::info("Using default audio device (-1)");
        BASS_Init(live_conf.audio_device_index, 44100, 0, 0, nullptr); // There's seems to be a problem here
    }
    else
    {
        bool bass_init_stat = BASS_Init(live_conf.audio_device_index, 44100, 0, 0, nullptr);
        if(!bass_init_stat)
            Playback::bassErrorHandler();
        else
        {
            int currentDeviceIndex       = BASS_GetDevice();
            live_conf.audio_device_index = currentDeviceIndex;
            BASS_DEVICEINFO deviceInfo;
            if(BASS_GetDeviceInfo(currentDeviceIndex, &deviceInfo))
                Log::info("", "BASS Successfully Initialized with audio device:\nName: %s\nDriver: %s\nDefault: %s\nEnabled: %s\nIndex: %d\n", deviceInfo.name, deviceInfo.driver, (deviceInfo.flags & BASS_DEVICE_DEFAULT) ? "Yes" : "No", (deviceInfo.flags & BASS_DEVICE_ENABLED) ? "Yes" : "No", currentDeviceIndex);
            else
                Log::warn("Failed to retrieve audio device information");
        }
    }

    // Let the user know if midi or soundfont files are missing
    if(!loaded_config.dont_show_missing_files)
    {
        if(SoundfontList::missing_files || MidiList::missing_files)
        {
            std::ostringstream msg;
            if(SoundfontList::missing_files)
                msg << "Soundfont files are missing";

            if(MidiList::missing_files)
                msg << "Previous MIDI files are missing";

            if(SoundfontList::missing_files && MidiList::missing_files)
                msg << "Previous MIDI files and SoundFonts are missing";


            SDL_MessageBoxButtonData buttons[] = {
                { 0,                                       0, "Don't Show Next Time!" },
                { 0,                                       1, "Show missing files"    },
                { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 2, "Ignore"                },
            };

            std::string        msg_str    = msg.str();
            SDL_MessageBoxData msgboxdata = {
                SDL_MESSAGEBOX_WARNING, RenderWin->Win, "Missing Files", msg_str.c_str(), SDL_arraysize(buttons), buttons,
                NULL // color scheme (optional)
            };

            int buttonid;
            if(SDL_ShowMessageBox(&msgboxdata, &buttonid) == 1)
            {
                switch(buttonid)
                {
                case 0: // Don't show next time
                    live_conf.dont_show_missing_files = true;
                    Config::Save(live_conf);
                    break;
                case 1:
                    break; // Show missing files (Todo)
                case 2:
                    break; // Ignore
                }
            }
        }
    }

    /* - - - - Graphics Rendering Setup - - - - */
    vis = new VisualizerHandler();



    Log::closeFile(); // Not yet needed
    return 0;
}