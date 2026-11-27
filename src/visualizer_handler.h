#ifndef VISUALIZER_ENTRY_H
#define VISUALIZER_ENTRY_H

#include <SDL3/SDL.h>
#include "mb_types.h"


class VisualizerHandler
{
  public:
    bool keep_running;
    using RenderFN = void (VisualizerHandler::*)();
    enum Type
    {
        Legacy = 1, // SDL3 Renderer
        GPU,    // SDL_gpu Renderer
        Custom
    };
    VisualizerHandler();
    ~VisualizerHandler();

    bool isDesktopSession();

    RenderFN RenderPFN = nullptr;

    void Render()
    {
        if(RenderPFN)
            (this->*RenderPFN)();
    }

    void setType(Type type);
    //static void shutdown();
    Type getType() const;

  private:
      int       vis_type_num = 0;
      
      SDL_Event Evt;
      u32       frameStart;
      int       frameRate = 60;
      
      void ImplSDL_RenderVisLoop();
      void ImplSDL_GPU_RenderVisLoop();
      const char        *video_driver;
};

#endif