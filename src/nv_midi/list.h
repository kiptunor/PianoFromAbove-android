#ifndef LIST_H
#define LIST_H




#include "seq.h"

#include <stack>
#include <list>

struct NVnote    /* ===== Note class rendering ===== */
{
    NVMidi::u16_t   track;
    double          Tstart, Tend;
    NVMidi::nv_byte chn, key, vel;

    NVnote(double T, const NVseq_event &E);
};

class NVnoteList  /* ===== Note queue class ===== */
{
public:

    NVmidiFile MIDI_File;      // MIDI File
    double Tread;      // Current read position
    std::list<NVnote> Note_list[128]; // Note List

    /* MIDI Parsing */
    bool start_parse(const char *name);

    /* Close component */
    void destroy_all();

    /* Locate to T seconds and clear the list */
    void list_seek(double T);

    /* Put the notes before the Tth second into the list */
    void update_to(double T);

    void OR();  // ppl in the Black MIDI Community knows what that means lol

    /* Remove notes in the list up to T seconds ago */
    void remove_to(double T);

private:

    NVsequencer Evt_sequencer;       // Event sequencer
    double     dT;       // Speed
    NVMidi::u32_t abstick;  // Current read position in ticks
    std::stack<std::list<NVnote>::iterator> (*keys)[128];
};
#endif