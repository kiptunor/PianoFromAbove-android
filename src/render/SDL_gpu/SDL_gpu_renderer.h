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

        bool IsInitialized() const { return device != nullptr; }
        SDL_GPUDevice *Device() const { return device; }
        SDL_GPUTextureFormat TargetFormat() const { return target_format; }
        SDL_GPUPresentMode PresentMode() const { return present_mode; }

        void RequestVSync(bool enabled);
        void ApplyPendingPresentMode();

        void PushNotes();
        void Render();

    private:
        
        SDL_Window *current_window;
        SDL_GPUTextureFormat   target_format   = SDL_GPU_TEXTUREFORMAT_INVALID;
        SDL_GPUPresentMode     present_mode    = SDL_GPU_PRESENTMODE_VSYNC;
        SDL_GPUPresentMode     pending_present_mode = SDL_GPU_PRESENTMODE_VSYNC;
};
#endif