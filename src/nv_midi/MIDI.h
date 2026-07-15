#ifndef MIDI_H
#define MIDI_H


#include "utils.h"














enum class NV_METYPE /* === MIDI Event types === */
{
    NOTE_OFF        = (NVMidi::nv_byte)0x80, // Noteoff
    NOTE_ON         = (NVMidi::nv_byte)0x90, // Noteon
    POLY_AFTERTOUCH = (NVMidi::nv_byte)0xA0, // Polifonic Key Pressure
    CTRL_CHANGE     = (NVMidi::nv_byte)0xB0, // ControlChange
    PROG_CHANGE     = (NVMidi::nv_byte)0xC0, // ProgramChange
    CHAN_PRESSURE   = (NVMidi::nv_byte)0xD0, // Channel
    CHAN_PREFIX     = (NVMidi::nv_byte)0x20, // Channel Prefix
    PITCH_BEND      = (NVMidi::nv_byte)0xE0, // Pitchbend
    SYST_EXCL       = (NVMidi::nv_byte)0xF0, // System Exclusive
    META            = (NVMidi::nv_byte)0xFF, // Meta-Event
    TEMPO_CHANGE    = (NVMidi::nv_byte)0x51, // Tempo Change
    TIME_SIGNATURE  = (NVMidi::nv_byte)0x58, // Time Signature
    MARKER          = (NVMidi::nv_byte)0x06, // Marker
    INSTR_NAME      = (NVMidi::nv_byte)0x04, // Instrument Name
    TRACK_NAME      = (NVMidi::nv_byte)0x03, // Track Name
    COPYRIGHT       = (NVMidi::nv_byte)0x02, // Copyright
    TEXT            = (NVMidi::nv_byte)0x01, // Text
    SMPTE           = (NVMidi::nv_byte)0x54, // Video sync
    SEQ_NUMBER      = (NVMidi::nv_byte)0x00, // Sequence Number
    SEQUENCER_SPEC  = (NVMidi::nv_byte)0x7F, // Sequencer Specific
    KEY_SIGNATURE   = (NVMidi::nv_byte)0x59, // Key Signature
};

struct NVmidiFile /* ===== MIDI file type ===== */
{
    /* Type, number of tracks, resolution */
    NVMidi::u16_t     type, tracks, current_loaded_track, ppnq;

    bool             *trk_over; // End of track
    NVMidi::nv_byte **trk_data; // Orbital data
    NVMidi::nv_byte **trk_ptr;  // Readout position
    NVMidi::nv_byte  *grp_code; // Event group

    /* Opening MIDI File */
    bool              mid_open(const char *name);

    void              rewind_all(); // Reset track pointers

    void              mid_close(); // Close midi file
};

struct NVmidiEvent /* =====  MIDI Event Class ===== */
{
    NV_METYPE              type;
    NVMidi::u32_t          tick;
    NVMidi::nv_byte        chan, num;
    NVMidi::u16_t          value;
    NVMidi::size_t         datasz;
    const NVMidi::nv_byte *data;

    /* Get events from a specified track of a specified file */
    bool                   get(NVMidi::u16_t track, NVmidiFile &midi);
};
#endif
