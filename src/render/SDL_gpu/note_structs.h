#ifndef NOTE_STRUCT_H
#define NOTE_STRUCT_H



#include "../../mb_types.h"




#pragma pack(push, 1)
struct GpuNote {
    f32 start;      // seconds
    f32 length;     // seconds, max(Tend-Tstart, epsilon)
    u8  key;        // 0..127
    u8  colorIndex; // index into GPU color table (track/channel palette)
    u16 flags;      // bit0=sharp(optional), rest reserved/padding
};
#pragma pack(pop)
static_assert(sizeof(GpuNote) == 12);

struct GpuColor
{
    f32 r, g, b, a; // normalized 0..1
};

struct NoteUniforms {
    f32 tplay;
    f32 tscr;        // visible seconds, or use pps
    f32 winW;
    f32 winH;
    f32 pps;         // pixels per second (UI::live_note_speed or WinH/vis_Tscr)
    f32 kbHeight;    // keyboard height in pixels
    f32 pad0;
    f32 pad1;
    GpuColor colors[16]; // or 256 for custom/random palette strategy
};
#endif