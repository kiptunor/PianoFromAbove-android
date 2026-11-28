#include <algorithm>

#include <backend_render/imgui_impl_sdlgpu3.h>

#include "SDL_gpu_renderer.h"
#include "shaders/note_fs.h"
#include "shaders/note_vs.h"
#include "shaders/keyboard_vs.h"
#include "shaders/keyboard_fs.h"
#include "../../globals.h"
#include "../../logger.h"
#include "../ui.h"











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
        device         = nullptr;
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
    // NoteVS uses 2 storage buffers (Notes, KeyGeom) and 1 uniform buffer.
    pci.vertex_shader   = LoadShaderRaw(device, note_vs_raw, note_vs_raw_len, SDL_GPU_SHADERSTAGE_VERTEX, "NoteVS", 0, 2, 1);
    // NoteFS currently only uses interpolated inputs.
    pci.fragment_shader = LoadShaderRaw(device, note_fs_raw, note_fs_raw_len, SDL_GPU_SHADERSTAGE_FRAGMENT, "NoteFS", 0, 0, 0);
    pci.primitive_type  = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    
    // No traditional vertex input if using storage buffer + SV_VertexID/SV_InstanceID.
    pci.vertex_input_state.num_vertex_buffers = 0;
    pci.vertex_input_state.num_vertex_attributes = 0;
    
    SDL_GPUColorTargetDescription ctd{};
    ctd.format                            = SDL_GetGPUSwapchainTextureFormat(device, window);
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


    m_kbStateBuffer = [&]{
        SDL_GPUBufferCreateInfo bci{};
        bci.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
        bci.size = sizeof(KbKeyStateGpu) * 128;
        return SDL_CreateGPUBuffer(device, &bci);
    }();
    {
        SDL_GPUTransferBufferCreateInfo tci{};
        tci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tci.size = sizeof(KbKeyStateGpu) * 128;
        m_kbStateTransfer = SDL_CreateGPUTransferBuffer(device, &tci);
    }

    // Piano Keyboard
    SDL_GPUShader* kbVS = LoadShaderRaw(device, keyboard_vs_raw, keyboard_vs_raw_len, SDL_GPU_SHADERSTAGE_VERTEX, "KeyboardVS", 0, 0, 1);
    SDL_GPUShader* kbFS = LoadShaderRaw(device, keyboard_fs_raw, keyboard_fs_raw_len, SDL_GPU_SHADERSTAGE_FRAGMENT, "KeyboardFS", 0, 0, 0);


    SDL_GPUGraphicsPipelineCreateInfo kbPci{};
    kbPci.vertex_shader                            = kbVS;
    kbPci.fragment_shader                          = kbFS;
    kbPci.primitive_type                           = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    kbPci.vertex_input_state.num_vertex_buffers    = 0;
    kbPci.vertex_input_state.num_vertex_attributes = 0;
    kbPci.target_info.num_color_targets            = 1;
    kbPci.target_info.color_target_descriptions    = &ctd;
    m_kbPipeline                                   = SDL_CreateGPUGraphicsPipeline(device, &kbPci);

    SDL_ReleaseGPUShader(device, kbVS);
    SDL_ReleaseGPUShader(device, kbFS);

    is_init = true;
}

SDL_GPURenderer::~SDL_GPURenderer()
{
    if(device)
    {
        SDL_WaitForGPUIdle(device);

        if(m_noteBuffer)
        {
            SDL_ReleaseGPUBuffer(device, m_noteBuffer);
            m_noteBuffer = nullptr;
        }

        if(m_noteTransfer)
        {
            SDL_ReleaseGPUTransferBuffer(device, m_noteTransfer);
            m_noteTransfer = nullptr;
        }

        if(m_keyGeomBuffer)
        {
            SDL_ReleaseGPUBuffer(device, m_keyGeomBuffer);
            m_keyGeomBuffer = nullptr;
        }

        if(notePipeline)
        {
            SDL_ReleaseGPUGraphicsPipeline(device, notePipeline);
            notePipeline = nullptr;
        }

        if(m_kbPipeline)
        {
            SDL_ReleaseGPUGraphicsPipeline(device, m_kbPipeline);
            m_kbPipeline = nullptr;
        }

        if(m_kbStateBuffer)
        {
            SDL_ReleaseGPUBuffer(device, m_kbStateBuffer);
            m_kbStateBuffer = nullptr;
        }

        if(m_kbStateTransfer)
        {
            SDL_ReleaseGPUTransferBuffer(device, m_kbStateTransfer);
            m_kbStateTransfer = nullptr;
        }

        if(current_window)
            SDL_ReleaseWindowFromGPUDevice(device, current_window);

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

    if(!SDL_SetGPUSwapchainParameters(device, current_window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, pending_present_mode))
    {
        Log::error("Failed to set GPU present mode: %s", SDL_GetError());
        pending_present_mode = present_mode;
        return;
    }

    present_mode = pending_present_mode;
}

void SDL_GPURenderer::EnsureKeyGeom(u32 winW, u32 winH)
{
    static const short GenKeyX[] = { 0, 12, 18, 33, 36, 54, 66, 72, 85, 90, 105, 108 };
    
    static const us_char KeyMap[128] = {
        0,2,4,5,7,9,11,12,14,16,17,19,21,23,24,26,
        28,29,31,33,35,36,38,40,41,43,45,47,48,50,52,53,
        55,57,59,60,62,64,65,67,69,71,72,74,76,77,79,81,
        83,84,86,88,89,91,93,95,96,98,100,101,103,105,107,108,
        110,112,113,115,117,119,120,122,124,125,127,1,3,6,8,10,
        13,15,18,20,22,25,27,30,32,34,37,39,42,44,46,49,
        51,54,56,58,61,63,66,68,70,73,75,78,80,82,85,87,
        90,92,94,97,99,102,104,106,109,111,114,116,118,121,123,126,
    };
    const f32 SharpRatio = 0.65f;

    int KeyX[128];
    int KeyWidth[128];
    
    for(int i = 0; i < 128; i++)
        KeyX[i] = (i / 12 * 126 + GenKeyX[i % 12]) * (int)winW / 1350;
    
    for(int i = 0; i < 127; i++)
    {
        int val;
        switch(i % 12)
        {
            case 1:
            case 3:
            case 6:
            case 8:
            case 10:
                val = (int)winW * 9 / 1350;
                break;
            case 4:
            case 11:
                val = KeyX[i + 1] - KeyX[i];
                break;
            default: val = KeyX[i + 2] - KeyX[i];
            break;
        }
        KeyWidth[i] = val;
    }
    KeyWidth[127] = (int)winW - KeyX[127];

    // Build geometry indexed by MIDI key number (n.key / NVnote.key).
    struct KeyGeom { f32 x, w, isSharp, pad; };
    KeyGeom geom[128]{};

    for(int displayIdx = 0; displayIdx < 128; ++displayIdx)
    {
        int midiKey = KeyMap[displayIdx]; // chromatic key index used by KeyX
        // In this codebase, NVnote.key is the MIDI note number and is used as
        // KeyMap[displayIdx] in rendering; but the note buffer stores n.key.
        // We need to index by n.key. Build both: by midiKey and also by displayIdx
        // if needed. The safest is to fill by actual MIDI key value.
    }
    // Build by MIDI key number. Invert KeyMap: displayIdx -> midiKey.
    for(int displayIdx = 0; displayIdx < 128; ++displayIdx)
    {
        int midiKey = KeyMap[displayIdx];
        int j = midiKey;
        f32 x = (f32)KeyX[j];
        f32 w;
        bool sharp = (j % 12 == 1 || j % 12 == 3 || j % 12 == 6 || j % 12 == 8 || j % 12 == 10);
        if(sharp)
        {
            f32 fNudge = 0.2f;
            int n = j % 12;
            
            if(n == 1 || n == 6)
                fNudge = 0.203f;
                
            else if(n == 3 || n == 10)
                fNudge = 0.297f;
            else if(n == 8)
                fNudge = 0.278f;
            
            x -= (f32)KeyWidth[0] * (SharpRatio / 2.0f - fNudge);
            w = (f32)KeyWidth[0] * SharpRatio;
        }
        else
            w = (f32)KeyWidth[j];
        
        if(midiKey >= 0 && midiKey < 128)
        {
            geom[midiKey].x       = x;
            geom[midiKey].w       = w;
            geom[midiKey].isSharp = sharp ? 1.0f : 0.0f;
            geom[midiKey].pad     = 0.0f;
        }
    }

    u32 size = sizeof(geom);
    if(!m_keyGeomBuffer)
    {
        SDL_GPUBufferCreateInfo bci{};
        bci.usage              = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
        bci.size               = size;
        m_keyGeomBuffer        = SDL_CreateGPUBuffer(device, &bci);
        m_keyGeomCapacityBytes = size;
    }

    SDL_GPUTransferBufferCreateInfo tci{};
    tci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tci.size  = size;
    SDL_GPUTransferBuffer* tbuf = SDL_CreateGPUTransferBuffer(device, &tci);
    void* mapped                = SDL_MapGPUTransferBuffer(device, tbuf, false);
    SDL_memcpy(mapped, geom, size);
    SDL_UnmapGPUTransferBuffer(device, tbuf);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy     = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation src{ tbuf, 0 };
    SDL_GPUBufferRegion dst{ m_keyGeomBuffer, 0, size };
    SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, tbuf);
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
    u32 width = 0;
    u32 height = 0;
    if(!SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, current_window, &target, &width, &height) || !target)
    {
        // No swapchain texture was acquired, so the command buffer can be
        // retired safely. Never cancel after a texture has been acquired.
        swapchainTexture = nullptr;
        SDL_SubmitGPUCommandBuffer(cmd_buf);
        return;
    }

    swapchainTexture = target;

    ConsumePendingNotes();
    EnsureKeyGeom(width, height);

    ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, cmd_buf);

    SDL_GPUColorTargetInfo target_info = {
        .texture              = swapchainTexture,
        .mip_level            = 0,
        .layer_or_depth_plane = 0,
        .clear_color          = SDL_FColor { gui->clear_color.x, gui->clear_color.y, gui->clear_color.z, gui->clear_color.w },
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

    // Normal keyboard state
    for(int i = 0; i < 128; ++i)
    {
        KeyPress[i] = false;
        KeyColor[i] = 0xFFFFFFFF;
    }

    // Draw notes before UI.
    if(notePipeline && m_noteBuffer && m_noteCount > 0 && m_keyGeomBuffer && MidiPlayer)
    {
        f32 tscr = static_cast<f32>(vis_Tscr);
        
        if(tscr <= 0.0f && gui && gui->live_note_speed > 0)
            tscr = static_cast<f32>(height) / static_cast<f32>(gui->live_note_speed);
        
        if(tscr > 0.0f)
        {
            NoteUniforms u{};
            u.tplay = static_cast<f32>(MidiPlayer->Tplay);
            u.tscr  = tscr;
            u.winW  = static_cast<f32>(width);
            u.winH  = static_cast<f32>(height);
            u.pps   = static_cast<f32>(height) / tscr; // pixels per second, matches SDLRenderer
            u.kbHeight = (f32)width * 82.0f / 1000.0f;
            u.pad0 = 0.0f;
            u.pad1 = 0.0f;
            
            for(int i = 0; i < 16; ++i)
            {
                unsigned int c = live_conf.channel_colors[i];
                f32 r = ((c >> 16) & 0xFF) / 255.0f;
                f32 g = ((c >> 8) & 0xFF) / 255.0f;
                f32 b = (c & 0xFF) / 255.0f;
                u.colors[i] = {r, g, b, 1.0f};
            }

            SDL_PushGPUVertexUniformData(cmd_buf, 0, &u, sizeof(u));

            SDL_BindGPUGraphicsPipeline(render_pass, notePipeline);
            SDL_GPUBuffer* storageBufs[2] = { m_noteBuffer, m_keyGeomBuffer };
            SDL_BindGPUVertexStorageBuffers(render_pass, 0, storageBufs, 2);
            //SDL_DrawGPUPrimitives(render_pass, 6, m_noteCount, 0, 0)
            const f32 minStart = u.tplay - m_maxNoteLength;
            const f32 maxStart = u.tplay + u.tscr;
            
            auto drawRange = [&](u32 begin, u32 end)
            {
                if(begin >= end)
                    return;
                
                auto itBegin = m_noteStarts.begin() + begin;
                auto itEnd   = m_noteStarts.begin() + end;
                auto lo = std::lower_bound(itBegin, itEnd, minStart);
                auto hi = std::upper_bound(itBegin, itEnd, maxStart);
                
                if(lo < hi)
                {
                    u32 first = static_cast<u32>(lo - m_noteStarts.begin());
                    u32 count = static_cast<u32>(hi - lo);
                    SDL_DrawGPUPrimitives(render_pass, 6, count, 0, first);
                }
            };
            
            // White notes first, then black notes on top.
            drawRange(0, m_whiteNoteCount);
            drawRange(m_whiteNoteCount, m_noteCount);
        }
    }

    KbKeyStateGpu states[128]{};
    for(int i = 0; i < 128; ++i)
    {
        states[i].pressed = KeyPress[i] ? 1u : 0u;
        states[i].color   = KeyColor[i]; // must be 0xAABBGGRR as in DrawKeyBoard
    }
    void* mapped = SDL_MapGPUTransferBuffer(device, m_kbStateTransfer, true);
    SDL_memcpy(mapped, states, sizeof(states));
    SDL_UnmapGPUTransferBuffer(device, m_kbStateTransfer);
    
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd_buf);
    SDL_GPUTransferBufferLocation src{m_kbStateTransfer, 0};
    SDL_GPUBufferRegion dst{m_kbStateBuffer, 0, sizeof(states)};
    SDL_UploadToGPUBuffer(copy, &src, &dst, true);
    SDL_EndGPUCopyPass(copy);

    auto toABGR = [](us_int c) -> u32
    {
        u32 r = (c >> 16) & 0xFF;
        u32 g = (c >> 8) & 0xFF;
        u32 b = (c      ) & 0xFF;
        return 0xFF000000u | (b << 16) | (g << 8) | r;
    };

    const f32 tplay = static_cast<f32>(MidiPlayer->Tplay);
    for(u32 idx = 0; idx < 0 + 128; ++idx)
    { // or iterate visible range
        const GpuNote& n = m_notes[idx];
        if(n.start <= tplay && tplay < n.start + n.length)
        {
            u32 k = n.key;
            if(k < 128)
            {
                KeyPress[k] = true;
                u32 ci      = n.colorIndexFlags & 0xFFu;
                KeyColor[k] = toABGR(live_conf.channel_colors[ci % 16]);
            }
        }
    }

    if(m_kbPipeline && m_kbStateBuffer)
    {
        KbUniformsGpu ku{ (f32)width, (f32)height, 0, 0 };
        SDL_PushGPUVertexUniformData(cmd_buf, 0, &ku, sizeof(ku));
        SDL_BindGPUGraphicsPipeline(render_pass, m_kbPipeline);
        SDL_GPUBuffer* sb[1] = { m_kbStateBuffer };
        SDL_BindGPUVertexStorageBuffers(render_pass, 0, sb, 1);
        SDL_DrawGPUPrimitives(render_pass, KB_TOTAL_VERTS, 1, 0, 0);
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

    // ci.format must be the single format of the provided bytecode, not a bitmask.
    // These embedded shaders are SPIR-V.
    SDL_GPUShaderFormat supported = SDL_GetGPUShaderFormats(dev);
    if(supported & SDL_GPU_SHADERFORMAT_SPIRV)
        ci.format = SDL_GPU_SHADERFORMAT_SPIRV;
    else if(supported & SDL_GPU_SHADERFORMAT_DXIL)
        ci.format = SDL_GPU_SHADERFORMAT_DXIL;
    else if(supported & SDL_GPU_SHADERFORMAT_MSL)
        ci.format = SDL_GPU_SHADERFORMAT_MSL;
    else if(supported & SDL_GPU_SHADERFORMAT_METALLIB)
        ci.format = SDL_GPU_SHADERFORMAT_METALLIB;
    else
    {
        Log::error("No supported shader format for embedded shader");
        return nullptr;
    }

    ci.num_samplers        = numSamplers;
    ci.num_storage_buffers = numStorageBuffers;
    ci.num_uniform_buffers = numUniformBuffers;
    return SDL_CreateGPUShader(dev, &ci);
}

void SDL_GPURenderer::SetPendingNotes(std::vector<GpuNote>&& notes)
{
    Log::debug("GPU notes: SetPendingNotes count=%zu", notes.size());
    std::lock_guard<std::mutex> lock(m_pendingNotesMutex);
    m_pendingNotes    = std::move(notes);
    m_hasPendingNotes = true;
}

void SDL_GPURenderer::ConsumePendingNotes()
{
    std::vector<GpuNote> notes;
    {
        std::lock_guard<std::mutex> lock(m_pendingNotesMutex);
        
        if(!m_hasPendingNotes)
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
    if(m_noteBuffer)
    {
        SDL_ReleaseGPUBuffer(device, m_noteBuffer);
        m_noteBuffer = nullptr;
    }

    m_noteCount = static_cast<u32>(notes.size());
    m_noteStarts.clear();
    m_whiteNoteCount = 0;
    m_maxNoteLength = 0.0f;
    Log::debug("GPU notes: PushNotes count=%u", m_noteCount);
    if(m_noteCount == 0)
        return;

    auto isSharpKey = [](u32 k)
    {
        int n = static_cast<int>(k % 12);
        return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
    };

    m_noteStarts.resize(m_noteCount);
    for(u32 i = 0; i < m_noteCount; ++i)
    {
        const auto& n = notes[i];
        m_noteStarts[i] = n.start;
        
        if(n.length > m_maxNoteLength)
            m_maxNoteLength = n.length;
        
        if(!isSharpKey(n.key))
            ++m_whiteNoteCount;
    }

    const u32 size = m_noteCount * static_cast<u32>(sizeof(GpuNote));

    SDL_GPUBufferCreateInfo bci{};
    bci.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
    bci.size  = size;
    m_noteBuffer = SDL_CreateGPUBuffer(device, &bci);

    SDL_GPUTransferBufferCreateInfo tci{};
    tci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tci.size  = size;
    SDL_GPUTransferBuffer* tbuf = SDL_CreateGPUTransferBuffer(device, &tci);

    void* mapped = SDL_MapGPUTransferBuffer(device, tbuf, false);
    SDL_memcpy(mapped, notes.data(), size);
    SDL_UnmapGPUTransferBuffer(device, tbuf);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy     = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation src{};
    src.transfer_buffer = tbuf;
    src.offset = 0;

    SDL_GPUBufferRegion dst{};
    dst.buffer = m_noteBuffer;
    dst.offset = 0;
    dst.size   = size;

    SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);

    SDL_ReleaseGPUTransferBuffer(device, tbuf);
}