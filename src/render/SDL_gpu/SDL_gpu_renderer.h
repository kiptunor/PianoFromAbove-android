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
        struct KbKeyStateGpu { u32 pressed; u32 color; u32 pad0; u32 pad1; };
        struct KbUniformsGpu { f32 winW, winH, pad0, pad1; };

        static constexpr u32 KB_TOTAL_PRIMS = 3 + 75*5 + 53*6; // 696
        static constexpr u32 KB_TOTAL_VERTS = KB_TOTAL_PRIMS * 6;
        
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
        void EnsureKeyGeom(u32 winW, u32 winH);
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
        SDL_GPUBuffer* m_keyGeomBuffer              = nullptr;
        u32 m_keyGeomCapacityBytes                  = 0;
        std::mutex m_pendingNotesMutex;
        std::vector<GpuNote> m_pendingNotes;
        bool m_hasPendingNotes = false;
        u32 m_noteTransferCapacityBytes             = 0;
        std::vector<GpuNote> m_visibleScratch;
        std::vector<u32> m_visibleIdxScratch;
        std::vector<u32> m_startSortedIdx;
        f32 m_maxNoteLength                         = 0.0f;

        SDL_GPUGraphicsPipeline* m_kbPipeline    = nullptr;
        SDL_GPUBuffer* m_kbStateBuffer           = nullptr;
        SDL_GPUTransferBuffer* m_kbStateTransfer = nullptr;

        f32 m_longNoteThresh                        = 10.0f;
        std::vector<u32> m_longNoteIdx;

        size_t m_sweepLo = 0;
        f32 m_lastTplay = -1.0f;


        bool KeyPress[128];
        u32  KeyColor[128];
        
        u32 m_whiteNoteCount                        = 0;
        std::vector<f32> m_noteStarts;
        std::vector<GpuNote> m_notes;               // CPU copy for visible-range upload
                
};
#endif