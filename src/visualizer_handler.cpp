#include <sstream>
#include <backend_render/imgui_impl_sdl3.h>





#include "visualizer_handler.h"
#include "render/SDLRenderer/note_buffer.h"
#include "render/SDLRenderer/SDLRenderer.h"
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

    keep_running = true;


    SDL_Init(SDL_INIT_VIDEO);

    
    video_driver = SDL_GetCurrentVideoDriver();


#ifdef PLATFORM_ANDROID
    main_win = SDL_CreateWindow("NVi-PFA", 1920, 1080, 0);
#else
    main_win = SDL_CreateWindow("NVi-PFA", 1912, 1000, SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
    SDL_SetWindowMinimumSize(main_win, 745, 716);
#endif


// Don't set window position on Wayland as it can throw errors on exit
#ifndef PLATFORM_ANDROID
    if(strcmp(video_driver, "wayland") != 0)
        SDL_SetWindowPosition(main_win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
#else
    SDL_SetWindowPosition(main_win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
#endif

     SDL_ShowWindow(main_win);



    // Create graphics rendering
    //DefaultRenderer = new SDLRenderer(main_win); // Only default for now

    GPU_Renderer = new SDL_GPURenderer(main_win);

    //UI::Setup(1);
    gui = new UI(2);


    // if(live_conf.background_image)
    //     DefaultRenderer->LoadBackgroundImage(live_conf.background_image_path);

    // RenderPFN = &VisualizerHandler::ImplSDL_RenderVisLoop;

    RenderPFN = &VisualizerHandler::ImplSDL_GPU_RenderVisLoop;

    /*
        Soon
        Render = &VisualizerHandler::SDL_GPU_RenderVisLoop;
    */
}

void VisualizerHandler::ImplSDL_RenderVisLoop()
{
    // Check if playback just ended (and we need to handle that)
    if(!MidiPlayer->playback_ended && MidiPlayer->IsMIDIEnded())
    {
        MidiPlayer->playback_ended = true;
        MidiPlayer->Tplay          = 1.0;  // Add a bit more to fully finish the note visualization
        MidiPlayer->is_paused      = true; // Just mark as paused when it ends

        // Save the position at the end
        MidiPlayer->UpdateEndPosition();

        if(loaded_config.audio_drv_name == "disk")
            au_stm->Close();
    }


    frameRate = UI::fps;

    // Set note size / note speed
    _WinH     = DefaultRenderer->WinH - DefaultRenderer->WinW * 80 / 1000;
    Tscr      = (double)_WinH / UI::live_note_speed;

    
    f64 tick_target = 1.0;
    
    if(smooth_tick_scale <= 0.0)
        smooth_tick_scale = tick_target;
    else
        smooth_tick_scale += (tick_target - smooth_tick_scale) * 0.12;
    f64 eff_scale = (smooth_tick_scale > 0.001) ? smooth_tick_scale : 1.0;
    vis_Tscr = Tscr / eff_scale;


    // Repeatedly call this function to start midi playback until the midi loader thread is finished
    MidiPlayer->UpdateState();



    // Start visualizing only if the synth's midi player is ready
    if(MidiPlayer->IsActive())
    {
        //Log::debug("updating midi vis");
        
        MidiPlayer->is_playback_started = true;
        Midi_ctx.update_to(MidiPlayer->Tplay + Tscr);
        Midi_ctx.remove_to(MidiPlayer->Tplay);
    }
    else
        MidiPlayer->is_playback_started = false;


    DefaultRenderer->clear();

    while(SDL_PollEvent(&Evt))
    {
        frameStart = SDL_GetTicks(); // Get the current time in milliseconds
        ImGui_ImplSDL3_ProcessEvent(&Evt);
        if(Evt.type == SDL_EVENT_QUIT)
            //shutdown();
            this->~VisualizerHandler();

        if(Evt.type == SDL_EVENT_WINDOW_RESIZED)
            DefaultRenderer->HandleResize(Evt.window.data1, Evt.window.data2);

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
                    MidiPlayer->Pause();
                    break;
                case SDLK_LEFT:
                    MidiPlayer->Seek_lazy(-MidiPlayer->seek_amount);
                    break;
                case SDLK_RIGHT:
                    MidiPlayer->Seek_lazy(MidiPlayer->seek_amount);
                    break;
                case SDLK_D: // Only for development purposes
                    UI::show_demo_window = true;
                    break;
                case SDLK_RETURN:
                    if(mods & SDL_KMOD_RALT)
                    {
                        ToggleFullscreen(DefaultRenderer->Win);
                    }
                    break;
                break;
                case SDLK_Q:
                    this->~VisualizerHandler();
                    break;
                }
            }
        }

        if(Evt.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            this->~VisualizerHandler();
#endif
    }

    // Set the background color again but with live color changes
    SDL_SetRenderDrawColor(DefaultRenderer->Ren, UI::liveColor.r, UI::liveColor.g, UI::liveColor.b, UI::liveColor.a);

    // Check whether the background image is enabled
    if(live_conf.background_image)
        SDL_RenderTexture(DefaultRenderer->Ren, DefaultRenderer->background_img, NULL, NULL);

    // and on top of the background image draw the vertical lines
    if(live_conf.draw_vertical_lines)
        DefaultRenderer->DrawBackgroundGrid();

    if(live_conf.draw_measure_lines)
        DefaultRenderer->DrawHorizontalLines();


    note_buf.clear();


    // Always draw notes

    if(MidiPlayer->is_playback_started) // This simple check makes the FPS to NOT drop after very high NPS
        for(int i = 0; i != 128; ++i)
        {
            for(const NVnote &n : Midi_ctx.Note_list[SDLRenderer::KeyMap[i]])
                // RenderWin->DrawNote(i, n, UI::live_note_speed); // Direct drawing
                note_buf.emplace_back(NoteBuffer::Note { NVMidi::u16_t(i), n, UI::live_note_speed }); // Layered drawing
            NoteBuffer::DrawNotes(live_conf);  // Available only if no direct note drawing is set
        }



    DefaultRenderer->DrawKeyBoard();  // Render the piano keyboard
    //UI::Render(DefaultRenderer->Ren); // Render the GUI

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
    SDL_RenderPresent(DefaultRenderer->Ren);

    // Only update Tplay if actively playing and not at the end
    if(!MidiPlayer->is_paused && !MidiPlayer->playback_ended && MidiPlayer->IsActive())
        MidiPlayer->UpdateMidiPos();
}

void VisualizerHandler::ImplSDL_GPU_RenderVisLoop()
{
    if(!MidiPlayer->playback_ended && MidiPlayer->IsMIDIEnded())
    {
        MidiPlayer->playback_ended = true;
        MidiPlayer->Tplay          = 1.0;  // Add a bit more to fully finish the note visualization
        MidiPlayer->is_paused      = true; // Just mark as paused when it ends

        // Save the position at the end
        MidiPlayer->UpdateEndPosition();

        if(loaded_config.audio_drv_name == "disk")
            au_stm->Close();
    }

    // Repeatedly call this function to start midi playback until the midi loader thread is finished
    MidiPlayer->UpdateState();
    

    while(SDL_PollEvent(&Evt))
    {
        frameStart = SDL_GetTicks(); // Get the current time in milliseconds
        ImGui_ImplSDL3_ProcessEvent(&Evt);
        if(Evt.type == SDL_EVENT_QUIT)
            //shutdown();
            //this->~VisualizerHandler();
            vis->keep_running = false;

        // if(Evt.type == SDL_EVENT_WINDOW_RESIZED)
        //     DefaultRenderer->HandleResize(Evt.window.data1, Evt.window.data2);

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
                    MidiPlayer->Pause();
                    break;
                case SDLK_LEFT:
                    MidiPlayer->Seek_lazy(-MidiPlayer->seek_amount);
                    break;
                case SDLK_RIGHT:
                    MidiPlayer->Seek_lazy(MidiPlayer->seek_amount);
                    break;
                case SDLK_D: // Only for development purposes
                    UI::show_demo_window = true;
                    break;
                case SDLK_RETURN:
                    if(mods & SDL_KMOD_RALT)
                    {
                        ToggleFullscreen(DefaultRenderer->Win);
                    }
                    break;
                break;
                case SDLK_Q:
                    this->~VisualizerHandler();
                    break;
                }
            }
        }

        if(Evt.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            vis->keep_running = false;
#endif
    }

    UI::Render();

    GPU_Renderer->Render();

    // Only update Tplay if actively playing and not at the end
    if(!MidiPlayer->is_paused && !MidiPlayer->playback_ended && MidiPlayer->IsActive())
        MidiPlayer->UpdateMidiPos();
}

bool VisualizerHandler::isDesktopSession()
{
    if(!video_driver)
        return false;
    return strcmp(video_driver, "wayland") == 0 || strcmp(video_driver, "x11") == 0;
}

VisualizerHandler::~VisualizerHandler()
{
    // Check first what visualizer is being used and clean up accordingly (Todo soon)
    keep_running = false;

    int exit_code = 0;
    //MidiPlayer->Close();
    Midi_ctx.destroy_all();

    const char *sdl_err = SDL_GetError();
    if(sdl_err && *sdl_err)
    {
        std::ostringstream temp;
        temp << "Caught last Error: " << sdl_err;

        if(isDesktopSession())
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Last SDL Error: ", temp.str().c_str(), nullptr);

        Log::error("", "SDL_GetError(): %s", sdl_err);
        exit_code = 1;
    }
    delete MidiPlayer;
    delete au_stm;
    delete gui;
    delete GPU_Renderer;
    
    //delete DefaultRenderer; // Its object deletion needs to be done selectively whether GPU rendering (SDL_gpu) is used instead

    SDL_DestroyWindow(main_win);
    SDL_Quit();

    // No dangling pointer class
    MidiPlayer       = nullptr;
    au_stm           = nullptr;
    //DefaultRenderer  = nullptr;
    GPU_Renderer     = nullptr;
    gui              = nullptr;
    
    Log::closeFile();
    //exit(exit_code);
}