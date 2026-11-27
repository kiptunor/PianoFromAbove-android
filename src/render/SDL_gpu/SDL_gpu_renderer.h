#ifndef SDL_GPU_RENDERER_H
#define SDL_GPU_RENDERER_H

#include <vector>
#include <mutex>

#include <SDL3/SDL.h>

#include "../../mb_types.h"
#include "note_structs.h"


class SDL_GPURenderer
{
    public:
        static SDL_GPUDevice* device;
        static SDL_GPUTexture* swapchainTexture;
        
        SDL_GPURenderer(SDL_Window *window);
        ~SDL_GPURenderer();

        bool IsInit() { return is_init; };

        SDL_GPUDevice *Device() const { return device; }
        SDL_GPUTextureFormat TargetFormat() const { return target_format; }
        SDL_GPUPresentMode PresentMode() const { return present_mode; }

        SDL_GPUShader* LoadShaderRaw(SDL_GPUDevice* dev, const void* code, size_t codeSize, SDL_GPUShaderStage stage, const char* entrypoint, u32 numSamplers = 0, u32 numStorageBuffers = 0, u32 numUniformBuffers = 0);

        void RequestVSync(bool enabled);
        void ApplyPendingPresentMode();

        void SetPendingNotes(std::vector<GpuNote>&& notes); // load thread
        void ConsumePendingNotes();
        void PushNotes(std::vector<GpuNote>&& notes);
        void Render();

    private:
        bool is_init = false;
        SDL_Window            *current_window;
        SDL_GPUTextureFormat   target_format        = SDL_GPU_TEXTUREFORMAT_INVALID;
        SDL_GPUPresentMode     present_mode         = SDL_GPU_PRESENTMODE_VSYNC;
        SDL_GPUPresentMode     pending_present_mode = SDL_GPU_PRESENTMODE_VSYNC;
        SDL_GPUGraphicsPipeline* notePipeline;
        SDL_GPUBuffer* m_noteBuffer                 = nullptr;
        SDL_GPUTransferBuffer* m_noteTransfer       = nullptr;
        u32 m_noteCount                             = 0;
        u32 m_noteCapacityBytes                     = 0;
        std::mutex m_pendingNotesMutex;
        std::vector<GpuNote> m_pendingNotes;
        bool m_hasPendingNotes = false;
};
#endif