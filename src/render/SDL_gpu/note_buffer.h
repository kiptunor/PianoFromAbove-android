#ifndef NOTE_BUFFER_H
#define NOTE_BUFFER_H

#include <SDL3/SDL.h>
#include <vector>
#include <unordered_map>
#include <utility>
#include "../../nv_midi/list.h"
#include "note_structs.h"

class GpuNoteBuffer {
public:
    void BuildFromMidi(NVnoteList& midi); // full pre-pass
    void Upload(SDL_GPUDevice* device);
    void Destroy(SDL_GPUDevice* device);

    SDL_GPUBuffer* Buffer() const { return noteBuffer; }
    Uint32 NoteCount() const { return static_cast<u32>(notes.size()); }

private:
    std::vector<GpuNote> notes;
    SDL_GPUBuffer* noteBuffer       = nullptr;
    SDL_GPUTransferBuffer* transfer = nullptr;
    Uint32 capacityBytes            = 0;

    static uint16_t ColorIndexFor(const NVnote& n, std::unordered_map<u64,u16>& map, u16& next);
};
#endif