#include <backend_render/imgui_impl_sdl3.h>
#include <imgui.h>
#include <sstream>
#ifdef PLATFORM_ANDROID
    #include <SDL3/SDL_system.h>
    #include <jni.h>
#endif





#include "visualizer_handler.h"
#include "video/exporter.h"
#include "globals.h"
#include "logger.h"
#include "render/note_buffer.h"
#include "render/render.h"
#include "render/ui.h"
#include "visualizer_handler.h"



int       vis_type_num = 0;

SDL_Event Evt;
u32       frameStart;
u64       tick_last_time   = 0;
int       frameRate        = 60;
int       frameRate        = 60;
f64       preRollStartTime = 0;






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



void VisualizerHandler::setType(Type type)
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
        if(!Playback::playback_ended && Playback::main_stream && BASS_ChannelIsActive(Playback::main_stream) == BASS_ACTIVE_STOPPED)
        {
            Playback::playback_ended = true;
            Playback::Tplay          = 1.0;  // Add a bit more to fully finish the note visualization
            Playback::is_paused      = true; // Just mark as paused when it ends

            // Finalize any active audio export
            VideoExporter::stop();

            // Save the position at the end
            Playback::saved_position = BASS_ChannelGetPosition(Playback::main_stream, BASS_POS_BYTE);
        }


        frameRate = UI::fps;

        // Set note size / note speed
        _WinH     = RenderWin->WinH - RenderWin->WinW * 80 / 1000;
        Tscr      = (double)_WinH / UI::live_note_speed;

        {
            f64 target = 1.0;
            if(live_conf.tick_based_playback && Midi_ctx.TempoCache.size() > 1)
            {
                f64 tempo = Midi_ctx.get_tempo_at_time(Playback::Tplay);
                f64 ref   = Midi_ctx.get_tempo_at_time(0.0);
                if(ref > 0.0) target = ref / tempo;
            }
            if(smooth_tick_scale <= 0.0)
                smooth_tick_scale = target;
            else
                smooth_tick_scale += (target - smooth_tick_scale) * 0.12;
            f64 eff_scale = (smooth_tick_scale > 0.001) ? smooth_tick_scale : 1.0;
            vis_Tscr = Tscr / eff_scale;
        }


        // Repeatedly call this function to start midi playback until the midi loader thread is finished
        Playback::PlayerStateUpdate();



        // Capture audio data during export
        VideoExporter::capture_audio();

        // Start visualizing only if bass thread is ready
        if(BASS_ChannelIsActive(Playback::main_stream))
        {
            if(!Playback::is_playback_started)
            {
                if(Playback::preRollActive)
                {
                    preRollStartTime = SDL_GetTicks();
                    Playback::Tplay  = -3.0;
                }
            }
            Playback::is_playback_started = true;
            Midi_ctx.update_to(Playback::Tplay + vis_Tscr);
            Midi_ctx.remove_to(Playback::Tplay);
        }
        else
        {
            Playback::is_playback_started = false;
            tick_last_time = 0;
            smooth_tick_scale = 0;
        }


        RenderWin->clear();

        while(SDL_PollEvent(&Evt))
        {
            frameStart = SDL_GetTicks(); // Get the current time in milliseconds

#ifndef PLATFORM_ANDROID
            // Handle player control keys BEFORE ImGui to prevent ImGui's keyboard
            // navigation (NavEnableKeyboard) from stealing them and causing
            // double-activation (e.g. pause called twice = no-op).
            if(!UI::main_gui_window && Evt.type == SDL_EVENT_KEY_DOWN)
            {
                SDL_Keymod mods = SDL_GetModState();
                switch(Evt.key.key)
                {
                case SDLK_SPACE:
                    Playback::pause();
                    continue;
                case SDLK_LEFT:
                    Playback::seek_playback(-Playback::seek_amount);
                    continue;
                case SDLK_RIGHT:
                    Playback::seek_playback(Playback::seek_amount);
                    continue;
                case SDLK_D:
                    UI::show_demo_window = true;
                    continue;
                case SDLK_RETURN:
                    if(mods & SDL_KMOD_RALT)
                        ToggleFullscreen(RenderWin->Win);
                    continue;
                case SDLK_Q:
                    shutdown();
                    continue;
                }
            }
#endif

            ImGui_ImplSDL3_ProcessEvent(&Evt);
            if(Evt.type == SDL_EVENT_QUIT)
                shutdown();

            if(Evt.type == SDL_EVENT_WINDOW_RESIZED)
                RenderWin->HandleResize(Evt.window.data1, Evt.window.data2);
        }

        // Set the background color again but with live color changes
        SDL_SetRenderDrawColor(RenderWin->Ren, UI::liveColor.r, UI::liveColor.g, UI::liveColor.b, UI::liveColor.a);

        // Check whether the background image is enabled
        if(live_conf.background_image)
            SDL_RenderTexture(RenderWin->Ren, RenderWin->background_img, NULL, NULL);

        // and on top of the background image draw the vertical lines
        if(live_conf.draw_vertical_lines)
            RenderWin->DrawBackgroundGrid();


        if(live_conf.draw_measure_lines)
            RenderWin->DrawHorizontalLines();


        note_buf.clear();


        // Always draw notes

        if(Playback::is_playback_started) // This simple check makes the FPS to NOT drop after very high NPS
            for(int i = 0; i != 128; ++i)
            {
                for(const NVnote &n : Midi_ctx.Note_list[Render::KeyMap[i]])
                    // RenderWin->DrawNote(i, n, UI::live_note_speed); // Direct drawing
                    note_buf.emplace_back(NoteBuffer::Note { NVMidi::u16_t(i), n, UI::live_note_speed }); // Layered drawing
                NoteBuffer::DrawNotes(live_conf);                                                         // Available only if no direct note drawing is set
            }



        RenderWin->DrawKeyBoard();  // Render the piano keyboard
        UI::Render(RenderWin->Ren); // Render the GUI

#ifdef PLATFORM_ANDROID
        // Update Android native stats overlay via JNI (only when playback is active)
        if(Playback::is_playback_started)
        {
            JNIEnv *env      = (JNIEnv *)SDL_GetAndroidJNIEnv();
            jobject activity = (jobject)SDL_GetAndroidActivity();
            if(env && activity)
            {
                jclass clazz = env->GetObjectClass(activity);
                if(clazz)
                {
                    f64  total = Playback::GetTotalTime();
                    int  curM  = (int)(Playback::Tplay / 60);
                    int  curS  = std::abs((int)Playback::Tplay % 60);
                    int  curT  = std::abs((int)((Playback::Tplay - floor(Playback::Tplay)) * 10));
                    int  totM  = (int)(total / 60);
                    int  totS  = (int)total % 60;
                    int  totT  = (int)((total - floor(total)) * 10);

                    char timeStr[48];
                    if(Playback::preRollActive)
                        snprintf(timeStr, sizeof(timeStr), "-%d:%02d.%d / %d:%02d.%d", curM, curS, curT, totM, totS, totT);
                    else
                        snprintf(timeStr, sizeof(timeStr), "%d:%02d.%d / %d:%02d.%d", curM, curS, curT, totM, totS, totT);

                    jstring   jTime  = env->NewStringUTF(timeStr);
                    jmethodID method = env->GetStaticMethodID(clazz, "updateTime", "(Ljava/lang/String;)V");
                    if(method)
                        env->CallStaticVoidMethod(clazz, method, jTime);
                    env->DeleteLocalRef(jTime);

                    // Update FPS once per second
                    {
                        static u64 lastFpsUpdate = 0;
                        u64        now           = SDL_GetTicks();
                        if(now - lastFpsUpdate >= 1000)
                        {
                            lastFpsUpdate = now;
                            char fpsStr[16];
                            snprintf(fpsStr, sizeof(fpsStr), "%.0f", ImGui::GetIO().Framerate);
                            jstring   jFps      = env->NewStringUTF(fpsStr);
                            jmethodID fpsMethod = env->GetStaticMethodID(clazz, "updateFps", "(Ljava/lang/String;)V");
                            if(fpsMethod)
                                env->CallStaticVoidMethod(clazz, fpsMethod, jFps);
                            env->DeleteLocalRef(jFps);
                        }
                    }

                    env->DeleteLocalRef(clazz);
                }
                env->DeleteLocalRef(activity);
            }
        }
#endif

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
        {
            if(Playback::preRollActive && preRollStartTime > 0)
            {
                f64 elapsed = (SDL_GetTicks() - preRollStartTime) / 1000.0;
                if(elapsed >= 3.0)
                {
                    Playback::preRollActive = false;
                    preRollStartTime        = 0;
                    BASS_ChannelSetAttribute(Playback::main_stream, BASS_ATTRIB_VOL, 1.0f);
                    BASS_ChannelPlay(Playback::main_stream, FALSE);
                    f64 bass_pos = BASS_ChannelBytes2Seconds(Playback::main_stream, BASS_ChannelGetPosition(Playback::main_stream, BASS_POS_BYTE));
                    Playback::clock_base_Tplay = bass_pos;
                    Playback::clock_start      = std::chrono::steady_clock::now();
                    Playback::clock_running    = true;
                    Playback::tick_position    = Midi_ctx.secondsToTick(bass_pos);
                    Playback::tick_accumulator = 0.0;
                    Playback::tick_last_frame  = std::chrono::steady_clock::now();
                    Playback::Tplay            = bass_pos;
                }
                else
                    Playback::Tplay = elapsed - 3.0;
            }
            else
            {
                if(!Playback::clock_running)
                {
                    f64 bass_pos = BASS_ChannelBytes2Seconds(Playback::main_stream, BASS_ChannelGetPosition(Playback::main_stream, BASS_POS_BYTE));
                    Playback::clock_base_Tplay = bass_pos;
                    Playback::clock_start      = std::chrono::steady_clock::now();
                    Playback::clock_running    = true;
                    Playback::tick_position    = Midi_ctx.secondsToTick(bass_pos);
                    Playback::tick_accumulator = 0.0;
                    Playback::tick_last_frame  = std::chrono::steady_clock::now();
                }
                auto now   = std::chrono::steady_clock::now();
                f64  dt    = std::chrono::duration<double>(now - Playback::clock_start).count();
                Playback::Tplay = Playback::clock_base_Tplay + dt;

                f64 bass_pos = BASS_ChannelBytes2Seconds(Playback::main_stream, BASS_ChannelGetPosition(Playback::main_stream, BASS_POS_BYTE));
                if(fabs(Playback::Tplay - bass_pos) > 0.05)
                {
                    Playback::clock_base_Tplay = bass_pos;
                    Playback::clock_start      = std::chrono::steady_clock::now();
                }
            }

            Playback::updateTickClock();
        }
    }
}

VisualizerHandler::~VisualizerHandler()
{
    // Check first what visualizer is being used and clean up accordingly

    shutdown();
}