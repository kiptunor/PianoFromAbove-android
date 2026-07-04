#include "../logger.h"

#include "MIDI.h"
#include "seq.h"
#include "utils.h"














void NVsequencer::seq_start(NVmidiFile &midi)
{
    Ev_buffer                      = new NVseq_event[midi.tracks + 1u];
    Ev_buffer[midi.tracks].abstick = 0xFFFFFFFFu;
    Ev_buffer[midi.tracks].track   = midi.tracks;

    for(Nodes = 1; Nodes < midi.tracks; Nodes <<= 1)
    {
    }

    Tree = new NVMidi::u16_t[Nodes << 1];
    seq_reset(midi);
}

void NVsequencer::seq_reset(NVmidiFile &midi)
{
    for(NVMidi::u32_t i = midi.tracks; i < Nodes; i++)
    {
        Tree[i + Nodes] = midi.tracks;
    }

    for(NVMidi::u16_t i = 0; i < midi.tracks; i++)
    {
        if(Ev_buffer[i].get(i, midi))
        {
            Ev_buffer[i].track = Tree[Nodes + i] = i;
            Ev_buffer[i].abstick                 = Ev_buffer[i].tick;
        }
        else
        {
            Tree[Nodes + i] = midi.tracks;
            Log::warn("Empty track: %d", i);
        }
    }

    for(int i = Nodes - 1; i > 0; --i)
    {
        update(i, Tree[i << 1], Tree[i << 1 | 1]);
    }
}

void NVsequencer::seq_next(NVmidiFile &midi)
{
    int a = Tree[1], p = Nodes + a;

    if(midi.trk_over[a] || !Ev_buffer[a].get(a, midi))
    {
        Tree[p] = midi.tracks;
    }
    else
    {
        Ev_buffer[a].abstick += Ev_buffer[a].tick;
    }

    for(; (a = p >> 1); p = a)
    {
        update(a, Tree[p], Tree[p ^ 1]);
    }
}

const NVseq_event &NVsequencer::event() const
{
    return Ev_buffer[Tree[1]];
}

void NVsequencer::seq_destroy()
{
    delete[] Tree;
    delete[] Ev_buffer;
    Tree      = nullptr;
    Ev_buffer = nullptr;
}

void NVsequencer::update(int p, int a, int b)
{
    Tree[p] = Ev_buffer[a].abstick < Ev_buffer[b].abstick ? a : b;
}
