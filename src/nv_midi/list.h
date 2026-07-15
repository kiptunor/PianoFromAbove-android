#ifndef LIST_H
#define LIST_H




#include "../mb_types.h"
#include "seq.h"

#include <list>
#include <stack>
#include <vector>














struct NVnote /* ===== Note class rendering ===== */
{
    NVMidi::u16_t   track;
    f64             Tstart, Tend;
    NVMidi::nv_byte chn, key, vel;

    NVnote(f64 T, const NVseq_event &E);
};

struct NVtempoEvent /* ===== Tempo change for grid rendering ===== */
{
    uint64_t tick;      // Absolute tick when this tempo takes effect
    f64      T;         // Time in seconds
    f64      usPerQuarter; // Microseconds per quarter note
};

struct TempoSegment
{
    uint64_t startTick;
    uint64_t endTick;      // exclusive
    double   startTime;    // seconds
    double   usPerQuarter;
    double   secondsPerTick;
    f64 T;            // Time in seconds
    f64 usPerQuarter; // Microseconds per quarter note
};

class NVnoteList /* ===== Note queue class ===== */
{
  public:
    NVmidiFile                MIDI_File;      // MIDI File
    f64                       Tread;          // Current read position
    std::list<NVnote>         Note_list[128]; // Note List

    /* MIDI Parsing */
    bool                      start_parse(const char *name);

    /* Tempo map for horizontal grid lines */
    std::vector<NVtempoEvent> TempoEvents;

    /* Get the tempo (µs/qn) at a given time in seconds */
    f64               get_tempo_at_time(f64 t) const;

    /* Tempo cache for O(log n) tick↔seconds conversion */
    void              build_tempo_cache();
    std::vector<TempoSegment> TempoCache;
    double            tickToSeconds(uint64_t tick) const;
    uint64_t          secondsToTick(double seconds) const;
    f64               get_tempo_at_tick(uint64_t tick) const;

    /* Close component */
    void                      destroy_all();

    /* Locate to T seconds and clear the list */
    void                      list_seek(f64 T);

    /* Put the notes before the Tth second into the list */
    void                      update_to(f64 T);

    void                      OR(); // ppl in the Black MIDI Community knows what that means lol

    /* Remove notes in the list up to T seconds ago */
    void                      remove_to(f64 T);

  private:
    NVsequencer                             Evt_sequencer; // Event sequencer
    f64                                     dT;            // Speed
    NVMidi::u32_t                           abstick;       // Current read position in ticks
    std::stack<std::list<NVnote>::iterator> (*keys)[128];
};
#endif
