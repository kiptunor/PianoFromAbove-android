#ifndef SEQ_H
#define SEQ_H




#include "MIDI.h"

struct NVseq_event: /* Sequencer MIDI event class */
    public NVmidiEvent
    {
        NVMidi::u16_t track;   // Orbital number
        
        NVMidi::u32_t abstick; // Absolute Tick
    };

class NVsequencer /* == sequencer class (computing) == */
{
public:

    /* Initialize this serializer */
    void seq_start(NVmidiFile &midi);

    /* Reset this Sequencer */
    void seq_reset(NVmidiFile &midi);

    /* Get the next MIDI event */
    void seq_next(NVmidiFile &midi);

    /* Current MIDI events */
    const NVseq_event& event() const;

    /* Close this Sequencer */
    void seq_destroy();

private:

    NVMidi::u32_t   Nodes;     // Maximum number of nodes
    NVMidi::u16_t  *Tree;      // Choice Tree
    NVseq_event    *Ev_buffer; // event buffer

    /* Update the specified node of the choice tree */
    void update(int p, int a, int b);
};

#endif