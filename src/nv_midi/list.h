#ifndef LIST_H
#define LIST_H




#include "../mb_types.h"
#include "seq.h"

#include <vector>
#include <list>
//#include <vector>
#include <stack>














struct NVnote /* ===== Note class rendering ===== */
{
    NVMidi::u16_t   track;
    f64             Tstart, Tend;
    NVMidi::nv_byte chn, key, vel;

    NVnote(f64 T, const NVseq_event &E);
};

struct NVtempoEvent /* ===== Tempo change for grid rendering ===== */
{
    u64 tick;         // Absolute tick when this tempo takes effect
    f64 T;            // Time in seconds
    f64 usPerQuarter; // Microseconds per quarter note
};

struct TempoSegment
{
    u64 startTick;
    u64 endTick;        // exclusive
    f64 startTime;      // seconds
    f64 secondsPerTick;
    f64 T;              // Time in seconds
    f64 usPerQuarter;   // Microseconds per quarter note
};

class NVnoteList /* ===== Note queue class ===== */
{
  public:
    NVmidiFile        MIDI_File;      // MIDI File
    f64               Tread;          // Current read position
    std::list<NVnote> Note_list[128]; // Note List

    /* MIDI Parsing */
    bool              start_parse(const char *name);

    std::vector<NVtempoEvent> TempoEvents;
    std::vector<TempoSegment> TempoCache;
    f64               get_tempo_at_time(f64 t) const;
    void              build_tempo_cache();
    f64            tickToSeconds(uint64_t tick) const;
    u64          secondsToTick(double seconds) const;
    f64               get_tempo_at_tick(uint64_t tick) const;

    /* Close component */
    void              destroy_all();

    /* Locate to T seconds and clear the list */
    void              list_seek(f64 T);

    /* Put the notes before the Tth second into the list */
    void              update_to(f64 T);

    void              OR(); // ppl in the Black MIDI Community knows what that means lol

    /* Remove notes in the list up to T seconds ago */
    void              remove_to(f64 T);

  private:
    NVsequencer                             Evt_sequencer; // Event sequencer
    f64                                     dT;            // Speed
    NVMidi::u32_t                           abstick;       // Current read position in ticks
    std::stack<std::list<NVnote>::iterator> (*keys)[128];
};
#endif
