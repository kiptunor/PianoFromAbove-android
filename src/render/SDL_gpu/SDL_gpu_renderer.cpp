
#include <backend_render/imgui_impl_sdlgpu3.h>

#include "SDL_gpu_renderer.h"
#include "shaders/note_fs.h"
#include "shaders/note_vs.h"
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

    SDL_GPUGraphicsPipelineCreateInfo pci{};
    pci.vertex_shader   = LoadShaderRaw(device, note_vs_raw, note_vs_raw_len, SDL_GPU_SHADERSTAGE_VERTEX, "NoteVS");   // entrypoint "NoteVS"
    pci.fragment_shader = LoadShaderRaw(device, note_fs_raw, note_fs_raw_len, SDL_GPU_SHADERSTAGE_FRAGMENT, "NoteFS"); // entrypoint "NoteFS"
    pci.primitive_type  = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    
    // No traditional vertex input if using storage buffer + SV_VertexID/SV_InstanceID.
    pci.vertex_input_state.num_vertex_buffers = 0;
    pci.vertex_input_state.num_vertex_attributes = 0;
    
    SDL_GPUColorTargetDescription ctd{};
    ctd.format = SDL_GetGPUSwapchainTextureFormat(device, window);
    ctd.blend_state.enable_blend          = true;
    ctd.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    ctd.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    ctd.blend_state.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    ctd.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    ctd.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    ctd.blend_state.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;
    ctd.blend_state.color_write_mask      = 0xF;
    
    pci.target_info.num_color_targets = 1;
    pci.target_info.color_target_descriptions = &ctd;
    
    notePipeline = SDL_CreateGPUGraphicsPipeline(device, &pci);
    
    if(!notePipeline)
        Log::error("", "Failed to create graphics pipeline: %s", SDL_GetError());

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

    SDL_ReleaseGPUGraphicsPipeline(device, notePipeline);
    notePipeline = nullptr;
    
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

SDL_GPUShader* SDL_GPURenderer::LoadShaderRaw(SDL_GPUDevice* dev, const void* code, size_t codeSize, SDL_GPUShaderStage stage, const char* entrypoint, u32 numSamplers, u32 numStorageBuffers, u32 numUniformBuffers)
{
    SDL_GPUShaderCreateInfo ci{};
    ci.code                = static_cast<const Uint8*>(code);
    ci.code_size           = codeSize;
    ci.entrypoint          = entrypoint; // custom name
    ci.stage               = stage;
    ci.format              = SDL_GetGPUShaderFormats(dev); // pick one you shipped
    // Better: select exactly one format you have bytecode for.
    ci.num_samplers        = numSamplers;
    ci.num_storage_buffers = numStorageBuffers;
    ci.num_uniform_buffers = numUniformBuffers;
    return SDL_CreateGPUShader(dev, &ci);
}

void SDL_GPURenderer::SetPendingNotes(std::vector<GpuNote>&& notes)
{
    std::lock_guard<std::mutex> lock(m_pendingNotesMutex);
    m_pendingNotes = std::move(notes);
    m_hasPendingNotes = true;
}

void SDL_GPURenderer::ConsumePendingNotes()
{
    std::vector<GpuNote> notes;
    {
        std::lock_guard<std::mutex> lock(m_pendingNotesMutex);
        if (!m_hasPendingNotes)
            return;

        notes = std::move(m_pendingNotes);
        m_pendingNotes.clear();
        m_hasPendingNotes = false;
    }

    // Upload on the render/GPU thread.
    PushNotes(std::move(notes));
}

void SDL_GPURenderer::PushNotes(std::vector<GpuNote>&& notes)
{
    if (m_noteBuffer) {
            SDL_ReleaseGPUBuffer(device, m_noteBuffer);
            m_noteBuffer = nullptr;
        }
    
        m_noteCount = static_cast<Uint32>(notes.size());
        if (m_noteCount == 0) return;
    
        Uint32 size = m_noteCount * static_cast<Uint32>(sizeof(GpuNote));
    
        SDL_GPUBufferCreateInfo bci{};
        bci.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
        bci.size = size;
        m_noteBuffer = SDL_CreateGPUBuffer(device, &bci);
    
        SDL_GPUTransferBufferCreateInfo tci{};
        tci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tci.size = size;
        SDL_GPUTransferBuffer* tbuf = SDL_CreateGPUTransferBuffer(device, &tci);
    
        void* mapped = SDL_MapGPUTransferBuffer(device, tbuf, false);
        SDL_memcpy(mapped, notes.data(), size);
        SDL_UnmapGPUTransferBuffer(device, tbuf);
    
        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    
        SDL_GPUTransferBufferLocation src{};
        src.transfer_buffer = tbuf;
        src.offset = 0;
    
        SDL_GPUBufferRegion dst{};
        dst.buffer = m_noteBuffer;
        dst.offset = 0;
        dst.size = size;
    
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
        SDL_SubmitGPUCommandBuffer(cmd);
    
        SDL_ReleaseGPUTransferBuffer(device, tbuf);
}