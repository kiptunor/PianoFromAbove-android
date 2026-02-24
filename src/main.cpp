#include <iostream>
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


//#define DEBUG

#include "logger.h"
#include "config/config.h"
#include "config/midi_list.h"
#include "globals.h"
#include "audio/playback.h"
#include "render/render.h"
#include "render/ui.h"
#ifndef PLATFORM_ANDROID
    #include "cli.h"
#endif






SDL_Event Evt;
u32 frameStart;
int frameTime;
const int frameRate = 60; // Todo: Add setting to UI
const int frameDelay = 1000 / frameRate;
NVnoteList Midi_ctx;



void Exit()
{
    BASS_Free();
    BASS_PluginFree(0);
    Midi_ctx.destroy_all();
    SDL_DestroyMutex(bass_mutex);
    
    const char * sdl_err = SDL_GetError();
    if(strlen(sdl_err) << 1)
    {
        std::ostringstream temp;
        temp << "Caught last Error: " << sdl_err;
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Last SDL Error: ", temp.str().c_str() , nullptr);
        SDL_Log("SDL Error: %s", sdl_err);
    }
    delete RenderWin;
    Log::closeFile();
    exit(0);
}

void AudioSetup()
{
    BASS_PluginLoad(BASSMIDI_LIB, 0);
    BASS_SetConfig(BASS_CONFIG_BUFFER, 5000); // Set buffer size to 500ms
    BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 10);
    BASS_SetConfig(BASS_CONFIG_MIDI_AUTOFONT, 0);
    
    BASS_Init(-1, 44100, 0, 0, nullptr); // Initialize default device for now
}

void UI::UpdateWidgetValues()
{
    UI::clear_color = UI::Irgba2ImVec4(live_conf.bg_R, live_conf.bg_G, live_conf.bg_B, live_conf.bg_A);
    
    UI::liveColor.r = live_conf.bg_R;
    UI::liveColor.g = live_conf.bg_G;
    UI::liveColor.b = live_conf.bg_B;
    UI::liveColor.a = live_conf.bg_A;
    
    for(int i = 0; i < 16; i++)
        ui_chcolors[i] = UI::UIntToImVec4(live_conf.channel_colors[i]);
    
    UI::current_audio_dev = live_conf.audio_device_index;
    UI::loop_colors = live_conf.loop_colors;
    UI::overlap_remover = live_conf.OR;
    UI::velocity_filter = live_conf.vel_filter;
    UI::live_note_speed = live_conf.note_speed;
    UI::min_velocity = live_conf.vel_min;
    UI::max_velocity = live_conf.vel_max;
    UI::last_midi_path = live_conf.last_midi_path;
    UI::last_midi_file = live_conf.last_midi_file;
    UI::vsync = live_conf.vsync;
    UI::soundfont_paths = live_conf.extra_sf_paths;
    UI::last_sf_path = live_conf.last_sf_path;
    UI::no_midi_duplicates = live_conf.no_midi_duplicates;
    UI::no_soundfont_duplicates = live_conf.no_soundfont_duplicates;
    live_soundfont_list = loaded_soundfont_list;
}

int APP_ENTRY(int argc, char *argv[])
{
#ifndef PLATFORM_ANDROID
    // CLI Parsing
    if(argc > 1)
        CLI::parseArgs(argc, argv);
#endif
    
    
    /*
    - - - - Configuration Handling - - - -
    */
    if(!std::filesystem::exists(CONFIG_FILE_PATH))
        live_conf = default_settings;
    else
    {
        loaded_config = Config::Load();
        live_conf = loaded_config;
    }
    
    /* - - - - Soundfont List Handling - - - - */
    if(std::filesystem::exists(SOUNDFONT_LIST_PATH))
        loaded_soundfont_list = SoundfontList::Load();
    
    
    /* - - - - MIDI List Handling - - - - */
    if(std::filesystem::exists(MIDI_LIST_PATH))
    {
        loaded_midi_list = MidiList::load();
        live_midi_list = loaded_midi_list;
    }
    
    
    /* - - - - Graphics Rendering Setup - - - - */
    RenderWin = new Render();
    
    UI::Setup(RenderWin->Win, RenderWin->Ren);
    
    UI::UpdateWidgetValues();
    
    
    
    SDL_SetRenderDrawColor(RenderWin->Ren, live_conf.bg_R, live_conf.bg_G, live_conf.bg_B, live_conf.bg_A); // Set initial background color
    
    availableAudioDevices = Playback::GetAudioOutputs();
    
    AudioSetup();
    
    if(availableAudioDevices.size() == 0)
    {
        Log::warn("", "No audio devices found!!!");
        Log::info("", "Using default audio device (-1)");
        BASS_Init(-1, 44100, 0, 0, nullptr);
    }
    else
    {
        if(!BASS_Init(live_conf.audio_device_index, 44100, 0, 0, nullptr))
        {
            Playback::bassErrorHandler();
        }
        else
        {
            int currentDeviceIndex = BASS_GetDevice();
            BASS_DEVICEINFO deviceInfo;
            if(BASS_GetDeviceInfo(currentDeviceIndex, &deviceInfo))
                Log::info("", "BASS Successfully Initialized with audio device:\nName: %s\nDriver: %s\nDefault: %s\nEnabled: %s\nIndex: %d", deviceInfo.name, deviceInfo.driver, (deviceInfo.flags & BASS_DEVICE_DEFAULT) ? "Yes" : "No", (deviceInfo.flags & BASS_DEVICE_ENABLED) ? "Yes" : "No", currentDeviceIndex);
            else
                Log::warn("", "Failed to retrieve audio device information");
        }
    }
    
    /*
    - - - - Mainloop - - - -
    */
    while(1)
    {
        // Check if playback just ended (and we need to handle that)
		if(!Playback::playback_ended && BASS_ChannelIsActive(Playback::main_stream) == BASS_ACTIVE_STOPPED)
		{
			Playback::playback_ended = true;
			Playback::Tplay = 1.0;           // Add a bit more to fully finish the note visualization
			Playback::is_paused = true;      // Just mark as paused when it ends
			
			// Save the position at the end
			Playback::saved_position = BASS_ChannelGetPosition(Playback::main_stream, BASS_POS_BYTE);
		}
		
		_WinH = RenderWin->WinH - RenderWin->WinW * 80 / 1000;
		Tscr = (double)_WinH / UI::live_note_speed;
		

		
		// Start visualizing only if bass thread is ready
		if(BASS_ChannelIsActive(Playback::main_stream))
		{
		    Playback::is_playback_started = true;
		    Midi_ctx.update_to(Playback::Tplay + Tscr);
		    Midi_ctx.remove_to(Playback::Tplay);
		}
		else
		{
		    Playback::is_playback_started = false;
		}
		
		RenderWin->clear();
		
		while(SDL_PollEvent(&Evt))
		{
            frameStart = SDL_GetTicks();  // Get the current time in milliseconds
            ImGui_ImplSDL3_ProcessEvent(&Evt);
            if(Evt.type == SDL_EVENT_QUIT)
                Exit();
            
#ifndef PLATFORM_ANDROID
            // Allow keyboard input only if the main window is not on display
            // This avoids playback control interferance when typing into the search boxes
            if(!UI::main_gui_window)
            {
			    if(Evt.type == SDL_EVENT_KEY_DOWN) 
			    {
		    	    switch(Evt.key.key)
		    	    {
	    		        case SDLK_SPACE:
				        	Playback::pause();
				        	break;
	    		        case SDLK_LEFT:
				        	Playback::seek_playback(-Playback::seek_amount);
				        	break;
	    		        case SDLK_RIGHT:
				        	Playback::seek_playback(Playback::seek_amount);
				        	break;
						case SDLK_D:
						    UI::show_demo_window = true;
							break;
	    		        case SDLK_Q:
						    Exit();
				        	break;
		    	    }
			    }
            }
#endif
		}
		
		// Set the background color again but with live color changes
		SDL_SetRenderDrawColor(RenderWin->Ren, UI::liveColor.r, UI::liveColor.g, UI::liveColor.b, UI::liveColor.a);
		
		RenderWin->DrawBackgroundGrid();
		
		// Always draw notes
		for(int i = 0; i != 128; ++i)
		{
			for(const NVnote &n : Midi_ctx.Note_list[KeyMap[i]])
				RenderWin->DrawNote(i, n, UI::live_note_speed);	
		}
		
		RenderWin->DrawKeyBoard(); // Render the piano keyboard
		UI::Render(RenderWin->Ren); // Render the GUI
		
		// FPS Adjustment
		frameTime = SDL_GetTicks() - frameStart;
		if(frameDelay > frameTime)
            SDL_Delay(frameDelay - frameTime);  // Delay to maintain consistent frame rate
        
		// Display the rendered frames
        SDL_RenderPresent(RenderWin->Ren);
        
        // Only update Tplay if actively playing and not at the end
		if(!Playback::is_paused && !Playback::playback_ended)
			Playback::Tplay = BASS_ChannelBytes2Seconds(Playback::main_stream, BASS_ChannelGetPosition(Playback::main_stream, BASS_POS_BYTE));
    }
    
    Log::closeFile();
    return 0;
}