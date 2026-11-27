#include <algorithm>
#include <cmath>



#include "note_buffer.h"





static inline uint64_t TrackChanKey(uint16_t track, uint8_t chn)
{
    return (uint64_t(track) << 8) | chn;
}

uint16_t GpuNoteBuffer::ColorIndexFor(const NVnote& n, std::unordered_map<uint64_t,uint16_t>& map, uint16_t& next)
{
    uint64_t k = TrackChanKey(n.track, n.chn);
    auto [it, inserted] = map.emplace(k, next);
    
    if(inserted)
        ++next;
    
    return it->second;
}

void GpuNoteBuffer::BuildFromMidi(NVnoteList& midi)
{
    notes.clear();

    // Full sequencer walk. This uses the same parsed MIDI file; it does not
    // interfere with live Note_list if done on a fresh sequencer/pass.
    // Option A: add NVnoteList::CollectAllNotes(vector<GpuNote>& out).
    // Option B: temporarily seek/update_to end on a dedicated NVnoteList copy.

    std::unordered_map<u64, u16> colorMap;
    u16 nextColor = 0;

    // Simple approach: scan the existing parsed MIDI by rewinding a local
    // sequencer state. Pseudocode-ish; adapt to NVnoteList internals.
    midi.list_seek(0.0);
    const f64 endT = 1e12; // or Midi_ctx end time if available
    midi.update_to(endT);

    for(int key = 0; key < 128; ++key)
    {
        for(const NVnote& n : midi.Note_list[key])
        {
            f32 start = static_cast<f32>(n.Tstart);
            f32 end   = static_cast<f32>(n.Tend);
            
            if(!std::isfinite(end) || end <= start)
                end = start + 0.001f;
            
            GpuNote gn{};
            gn.start           = start;
            gn.length          = std::max(0.001f, end - start);
            gn.key             = static_cast<u32>(n.key);
            gn.colorIndexFlags = static_cast<u32>(ColorIndexFor(n, colorMap, nextColor) % 255u);
            notes.push_back(gn);
        }
    }

    // Optional: sort by start for chunked streaming/debugging; not required for instancing.
    std::sort(notes.begin(), notes.end(), [](auto& a, auto& b){ return a.start < b.start; });
}

void GpuNoteBuffer::Upload(SDL_GPUDevice* device)
{
    Destroy(device);
    
    if(notes.empty())
        return;

    Uint32 size = Uint32(notes.size() * sizeof(GpuNote));
    SDL_GPUBufferCreateInfo bci{};
    bci.usage  = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ; // vertex shader reads
    bci.size   = size;
    noteBuffer = SDL_CreateGPUBuffer(device, &bci);

    SDL_GPUTransferBufferCreateInfo tci{};
    tci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tci.size  = size;
    SDL_GPUTransferBuffer* tbuf = SDL_CreateGPUTransferBuffer(device, &tci);

    void* mapped = SDL_MapGPUTransferBuffer(device, tbuf, false);
    SDL_memcpy(mapped, notes.data(), size);
    SDL_UnmapGPUTransferBuffer(device, tbuf);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy     = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation src{ tbuf, 0 };
    SDL_GPUBufferRegion dst{ noteBuffer, 0, size };
    SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd); // or fence for async

    SDL_ReleaseGPUTransferBuffer(device, tbuf);
    // Optionally notes.clear(); notes.shrink_to_fit(); to save CPU RAM on mobile.
}

void GpuNoteBuffer::Destroy(SDL_GPUDevice* device)
{
    if(!device)
        return;

    if(noteBuffer)
    {
        SDL_ReleaseGPUBuffer(device, noteBuffer);
        noteBuffer = nullptr;
    }

    if(transfer)
    {
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        transfer = nullptr;
    }

    capacityBytes = 0;
    // noteCount/capacity members as you have them:
    // m_noteCount = 0;

    // Optional: free CPU-side copy if you don't need to re-upload.
    // notes.clear();
    // notes.shrink_to_fit();
}