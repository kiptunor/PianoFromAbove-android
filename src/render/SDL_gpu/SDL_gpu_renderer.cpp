
#include <backend_render/imgui_impl_sdlgpu3.h>

#include "SDL_gpu_renderer.h"
#include "../../logger.h"


SDL_GPUDevice* SDL_GPURenderer::device = nullptr;
SDL_GPUTexture* SDL_GPURenderer::swapchainTexture = nullptr;

SDL_GPURenderer::SDL_GPURenderer(SDL_Window *window)
{
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_METALLIB | SDL_GPU_SHADERFORMAT_MSL , true, NULL);

    if(!device)
    {
        Log::error("Failed to create GPU device!!!");
        return;
    }

    if(!SDL_ClaimWindowForGPUDevice(device, window))
    {
        Log::error("Failed to attach GPU device to window!!!");
        return;
    }

    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    current_window = window;

}

SDL_GPURenderer::~SDL_GPURenderer()
{
    /*
    SDL_WaitForGPUIdle(device);
    SDL_ReleaseWindowFromGPUDevice(device, current_window);
    SDL_DestroyGPUDevice(device);
    */

    if (device)
    {
        SDL_WaitForGPUIdle(device);

        if(current_window)
            SDL_ReleaseWindowFromGPUDevice(device,current_window);
        

        SDL_DestroyGPUDevice(device);
        device = nullptr;
    }
    
    current_window = nullptr;
    swapchainTexture = nullptr;
}


void SDL_GPURenderer::Render()
{
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

            
    SDL_GPUCommandBuffer* cmd_buf = SDL_AcquireGPUCommandBuffer(device);


    SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, current_window, &swapchainTexture, NULL, NULL);

    if(swapchainTexture != nullptr && !is_minimized)
    {
        ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, cmd_buf);
        
        SDL_GPUColorTargetInfo target = {
            .texture              = swapchainTexture,
            .mip_level            = 0,
            .layer_or_depth_plane = 0,
            .clear_color          = { 0.1f, 0.2f, 0.4f, 1.0f },
            .load_op              = SDL_GPU_LOADOP_CLEAR,
            .store_op             = SDL_GPU_STOREOP_STORE,
            .cycle                = false
        };

        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd_buf, &target, 1, nullptr);

        // Render ImGui
        ImGui_ImplSDLGPU3_RenderDrawData(draw_data, cmd_buf, render_pass);

        SDL_EndGPURenderPass(render_pass);
    }
    
    SDL_SubmitGPUCommandBuffer(cmd_buf);
}