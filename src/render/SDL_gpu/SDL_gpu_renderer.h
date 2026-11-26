#ifndef SDL_GPU_RENDERER_H
#define SDL_GPU_RENDERER_H


#include <SDL3/SDL.h>


class SDL_GPURenderer
{
    public:
        static SDL_GPUDevice* device;
        static SDL_GPUTexture* swapchainTexture;
        
        SDL_GPURenderer(SDL_Window *window);
        ~SDL_GPURenderer();

        void PushNotes();
        void Render();

    private:
        
        SDL_Window *current_window;
};
#endif