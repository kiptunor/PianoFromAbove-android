#ifndef MIDI_H
#define MIDI_H


#include "utils.h"

inline int total_track_count;

enum class NV_METYPE  /* === MIDI Event types === */
{
    NOFF = (NVMidi::nv_byte)0x80, // Noteoff
    NOON = (NVMidi::nv_byte)0x90, // Noteon
    NOAT = (NVMidi::nv_byte)0xA0, // Polifonic Key Pressure
    CTRO = (NVMidi::nv_byte)0xB0, // ControlChange
    PROG = (NVMidi::nv_byte)0xC0, // ProgramChange
    CHAT = (NVMidi::nv_byte)0xD0, // Channel
    PITH = (NVMidi::nv_byte)0xE0, // Pitchbend
    SYSC = (NVMidi::nv_byte)0xF0, // System Exclusive
    META = (NVMidi::nv_byte)0xFF, // Meta-Event
};

struct NVmidiFile   /* ===== MIDI file type ===== */
{
    /* Type, number of tracks, resolution */
    NVMidi::u16_t type, tracks, ppnq;

    bool             *trk_over;  // End of track
    NVMidi::nv_byte **trk_data;  // Orbital data
    NVMidi::nv_byte **trk_ptr;   // Readout position
    NVMidi::nv_byte  *grp_code;  // Event group

    /* Opening MIDI File */
    bool mid_open(const char *name);

    void rewind_all();  // Reset track pointers

    void mid_close();   // Close midi file
};

struct NVmidiEvent  /* =====  MIDI Event Class ===== */
{
    NV_METYPE       type;
    NVMidi::u32_t   tick;
    NVMidi::nv_byte chan, num;
    NVMidi::u16_t   value;
    NVMidi::size_t  datasz;
    const NVMidi::nv_byte *data;

    /* Get events from a specified track of a specified file */
    bool get(NVMidi::u16_t track, NVmidiFile &midi);
};
#endif