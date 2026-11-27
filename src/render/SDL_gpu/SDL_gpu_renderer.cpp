
#include <backend_render/imgui_impl_sdlgpu3.h>

#include "SDL_gpu_renderer.h"
#include "../../globals.h"
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

    current_window = window;

    target_format = SDL_GetGPUSwapchainTextureFormat(device, window);
    if(target_format == SDL_GPU_TEXTUREFORMAT_INVALID)
    {
        Log::error("Failed to get GPU swapchain texture format!!!");
        SDL_ReleaseWindowFromGPUDevice(device, current_window);
        SDL_DestroyGPUDevice(device);
        device = nullptr;
        current_window = nullptr;
        return;
    }
    
    SDL_GPUPresentMode mode = SDL_GPU_PRESENTMODE_VSYNC;
    if(!SDL_WindowSupportsGPUPresentMode(device, window, mode) && SDL_WindowSupportsGPUPresentMode(device, window, SDL_GPU_PRESENTMODE_IMMEDIATE))
        mode = SDL_GPU_PRESENTMODE_IMMEDIATE;

    if(!SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, mode))
    {
        Log::error("", "Failed to set initial GPU present mode: %s", SDL_GetError());
        mode = SDL_GPU_PRESENTMODE_VSYNC;
    }

    is_init = true;
}

SDL_GPURenderer::~SDL_GPURenderer()
{
    if(device)
    {
        SDL_WaitForGPUIdle(device);

        if(current_window)
            SDL_ReleaseWindowFromGPUDevice(device,current_window);
        

        SDL_DestroyGPUDevice(device);
        device = nullptr;
    }
    
    current_window   = nullptr;
    swapchainTexture = nullptr;
    target_format    = SDL_GPU_TEXTUREFORMAT_INVALID;
    present_mode     = pending_present_mode = SDL_GPU_PRESENTMODE_VSYNC;
}

void SDL_GPURenderer::RequestVSync(bool enabled)
{
    SDL_GPUPresentMode mode = enabled ? SDL_GPU_PRESENTMODE_VSYNC : SDL_GPU_PRESENTMODE_IMMEDIATE;

    if(device && current_window && !SDL_WindowSupportsGPUPresentMode(device, current_window, mode))
        mode = SDL_GPU_PRESENTMODE_VSYNC;

    pending_present_mode = mode;
}

void SDL_GPURenderer::ApplyPendingPresentMode()
{
    if(!device || !current_window || pending_present_mode == present_mode)
        return;

    if(!SDL_SetGPUSwapchainParameters(device, current_window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                      pending_present_mode))
    {
        Log::error("Failed to set GPU present mode: %s", SDL_GetError());
        pending_present_mode = present_mode;
        return;
    }

    present_mode = pending_present_mode;
}

void SDL_GPURenderer::Render()
{
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

    if(is_minimized)
        return;

    // Must happen before the swapchain texture is acquired for this frame.
    ApplyPendingPresentMode();
            
    SDL_GPUCommandBuffer* cmd_buf = SDL_AcquireGPUCommandBuffer(device);

    SDL_GPUTexture* target = nullptr;
    Uint32 width = 0;
    Uint32 height = 0;
    if(!SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, current_window, &target, &width, &height) || !target)
    {
        // No swapchain texture was acquired, so the command buffer can be
        // retired safely. Never cancel after a texture has been acquired.
        swapchainTexture = nullptr;
        SDL_SubmitGPUCommandBuffer(cmd_buf);
        return;
    }

    swapchainTexture = target;

    ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, cmd_buf);

    SDL_GPUColorTargetInfo target_info = {
        .texture              = swapchainTexture,
        .mip_level            = 0,
        .layer_or_depth_plane = 0,
        .clear_color = SDL_FColor { gui->clear_color.x, gui->clear_color.y, gui->clear_color.z, gui->clear_color.w },
        .load_op              = SDL_GPU_LOADOP_CLEAR,
        .store_op             = SDL_GPU_STOREOP_STORE,
        .cycle                = false
    };

    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd_buf, &target_info, 1, nullptr);
    if(!render_pass)
    {
        Log::error("Failed to begin GPU render pass: %s", SDL_GetError());
        // Cancelling after swapchain acquisition is invalid; submit instead.
        SDL_SubmitGPUCommandBuffer(cmd_buf);
        swapchainTexture = nullptr;
        return;
    }

    ImGui_ImplSDLGPU3_RenderDrawData(draw_data, cmd_buf, render_pass);

    SDL_EndGPURenderPass(render_pass);
    SDL_SubmitGPUCommandBuffer(cmd_buf);
    swapchainTexture = nullptr;
}