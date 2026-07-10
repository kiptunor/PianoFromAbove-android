#include <sstream>
#include <backend_render/imgui_impl_sdl3.h>





#include "visualizer_handler.h"
#include "render/note_buffer.h"
#include "render/render.h"
#include "render/ui.h"
#include "globals.h"
#include "logger.h"



int       vis_type_num = 0;

SDL_Event Evt;
u32       frameStart;
int       frameRate = 60;






#ifndef PLATFORM_ANDROID
void ToggleFullscreen(SDL_Window *window)
{
    if(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN)
        SDL_SetWindowFullscreen(window, false);
    else
    {
        SDL_SetWindowFullscreenMode(window, NULL); // use current display mode
        SDL_SetWindowFullscreen(window, true);
    }
}
#endif



void VisualizerHandler::shutdown()
{
    int exit_code = 0;
    BASS_Free();
    BASS_PluginFree(0);
    Midi_ctx.destroy_all();

    const char *sdl_err = SDL_GetError();
    if(strlen(sdl_err) << 1)
    {
        std::ostringstream temp;
        temp << "Caught last Error: " << sdl_err;

        if(Render::isDesktopSession())
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Last SDL Error: ", temp.str().c_str(), nullptr);

        Log::error("", "SDL_GetError(): %s", sdl_err);
        exit_code = 1;
    }
    delete RenderWin;
    Log::closeFile();
    exit(exit_code);
}



void      VisualizerHandler::setType(Type type)
{
    vis_type_num = type;
}

VisualizerHandler::Type VisualizerHandler::getType() const
{
    Type vis_type;

    // Todo

    return vis_type;
}

VisualizerHandler::VisualizerHandler()
{
    // Handle initialization of the GPU based visualizer first (Soon)
    // If not working fallback to the legacy visualizer
    // Then handle the mainloop, logic and GUI display



    RenderWin = new Render();

    UI::Setup(1);


    // Set initial background color
    SDL_SetRenderDrawColor(RenderWin->Ren, live_conf.bg_R, live_conf.bg_G, live_conf.bg_B, live_conf.bg_A); // Set initial background color

    if(live_conf.background_image)
        RenderWin->LoadBackgroundImage(live_conf.background_image_path);

    /*
    - - - - Mainloop - - - -
    */
    while(1)
    {
        // Check if playback just ended (and we need to handle that)
        if(!Playback::playback_ended && BASS_ChannelIsActive(Playback::main_stream) == BASS_ACTIVE_STOPPED)
        {
            Playback::playback_ended = true;
            Playback::Tplay          = 1.0;  // Add a bit more to fully finish the note visualization
            Playback::is_paused      = true; // Just mark as paused when it ends

            // Save the position at the end
            Playback::saved_position = BASS_ChannelGetPosition(Playback::main_stream, BASS_POS_BYTE);
        }


        frameRate = UI::fps;

        // Set note size / note speed
        _WinH     = RenderWin->WinH - RenderWin->WinW * 80 / 1000;
        Tscr      = (double)_WinH / UI::live_note_speed;


        // Repeatedly call this function to start midi playback until the midi loader thread is finished
        Playback::PlayerStateUpdate();



        // Start visualizing only if bass thread is ready
        if(BASS_ChannelIsActive(Playback::main_stream))
        {
            Playback::is_playback_started = true;
            Midi_ctx.update_to(Playback::Tplay + Tscr);
            Midi_ctx.remove_to(Playback::Tplay);
        }
        else
            Playback::is_playback_started = false;


        RenderWin->clear();

        while(SDL_PollEvent(&Evt))
        {
            frameStart = SDL_GetTicks(); // Get the current time in milliseconds
            ImGui_ImplSDL3_ProcessEvent(&Evt);
            if(Evt.type == SDL_EVENT_QUIT)
                shutdown();

            if(Evt.type == SDL_EVENT_WINDOW_RESIZED)
                RenderWin->HandleResize(Evt.window.data1, Evt.window.data2);

#ifndef PLATFORM_ANDROID
            // Allow keyboard input only if the main window is not on display
            // This avoids playback control interferance when typing into the search boxes
            if(!UI::main_gui_window)
            {
                if(Evt.type == SDL_EVENT_KEY_DOWN)
                {
                    SDL_Keymod mods = SDL_GetModState();
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
                    case SDLK_D: // Only for development purposes
                        UI::show_demo_window = true;
                        break;
                    case SDLK_RETURN:
                        if(mods & SDL_KMOD_RALT)
                        {
                            ToggleFullscreen(RenderWin->Win);
                        }
                        break;
                    case SDLK_Q:
                        shutdown();
                        break;
                    }
                }
            }
#endif
        }

        // Set the background color again but with live color changes
        SDL_SetRenderDrawColor(RenderWin->Ren, UI::liveColor.r, UI::liveColor.g, UI::liveColor.b, UI::liveColor.a);

        // Check whether the background image is enabled
        if(live_conf.background_image)
            SDL_RenderTexture(RenderWin->Ren, RenderWin->background_img, NULL, NULL);

        // and on top of the background image draw the vertical lines
        if(live_conf.draw_vertical_lines)
        {
            RenderWin->DrawBackgroundGrid();
            RenderWin->DrawHorizontalLines();
        }


        note_buf.clear();


        // Always draw notes

        if(Playback::is_playback_started) // This simple check makes the FPS to NOT drop after very high NPS
            for(int i = 0; i != 128; ++i)
            {
                for(const NVnote &n : Midi_ctx.Note_list[Render::KeyMap[i]])
                    // RenderWin->DrawNote(i, n, UI::live_note_speed); // Direct drawing
                    note_buf.emplace_back(NoteBuffer::Note { NVMidi::u16_t(i), n, UI::live_note_speed }); // Layered drawing
                NoteBuffer::DrawNotes(live_conf);  // Available only if no direct note drawing is set
            }



        RenderWin->DrawKeyBoard();  // Render the piano keyboard
        UI::Render(RenderWin->Ren); // Render the GUI

        // Outdated FPS adjustment
        // This also caused noticeable FPS drops on mouse movement across the screen in combination with imgui elements
        /*
        frameTime = SDL_GetTicks() - frameStart;
        if(frameDelay > frameTime)
            SDL_Delay(frameDelay - frameTime);  // Delay to maintain consistent frame rate
        */

        // FPS Adjustment
        u64 frame_start = SDL_GetTicksNS();
        u64 elapsed     = SDL_GetTicksNS() - frame_start;
        u64 target      = 1000000000ULL / frameRate;

        if(elapsed < target)
            SDL_DelayNS(target - elapsed);

        // Display the rendered frames
        SDL_RenderPresent(RenderWin->Ren);

        // Only update Tplay if actively playing and not at the end
        if(!Playback::is_paused && !Playback::playback_ended)
            Playback::Tplay = BASS_ChannelBytes2Seconds(Playback::main_stream, BASS_ChannelGetPosition(Playback::main_stream, BASS_POS_BYTE));
    }
}

VisualizerHandler::~VisualizerHandler()
{
    // Check first what visualizer is being used and clean up accordingly

    shutdown();
}