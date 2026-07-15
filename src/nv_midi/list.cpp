#include <algorithm>

#include "../logger.h"

#include "MIDI.h"
#include "list.h"
#include "seq.h"
#include "utils.h"














template <typename T> struct rP
{
    using t = T;
};
template <typename T> struct rP<T *>
{
    using t = T;
};

NVnote::NVnote(f64 T, const NVseq_event &E) : track(E.track), Tstart(T), Tend(114514191981.0), chn(E.chan), key(E.num), vel(E.value)
{
}

bool NVnoteList::start_parse(const char *name)
{
    if(!MIDI_File.mid_open(name))
    {
        return false;
    }

    if(MIDI_File.type == 2)
    {
        Log::error("Invalid MIDI File format !");
        Log::info("Format 0 / 1 are compatible.");
        return (MIDI_File.mid_close(), false);
    }

    Evt_sequencer.seq_start(MIDI_File);
    abstick = 0;
    Tread   = 0.0;
    dT      = 0.5 / MIDI_File.ppnq;
    keys    = new rP<decltype(keys)>::t[MIDI_File.tracks];
    TempoEvents.clear();
    TempoEvents.push_back({ 0, 0.0, 500000.0 });
    TempoCache.clear();
    return true;
}

f64 NVnoteList::get_tempo_at_time(f64 t) const
{
    if(TempoCache.empty()) return 500000.0;
    auto it = std::upper_bound(TempoCache.begin(), TempoCache.end(), t,
        [](double time, const TempoSegment &seg) { return time < seg.startTime; });
    if(it != TempoCache.begin()) --it;
    return it->usPerQuarter;
}

void NVnoteList::build_tempo_cache()
{
    TempoCache.clear();
    if(TempoEvents.empty())
    {
        TempoCache.push_back({ 0, UINT64_MAX, 0.0, 500000.0, 0.5 / MIDI_File.ppnq });
        return;
    }
    uint64_t ppnq = MIDI_File.ppnq;
    TempoCache.reserve(TempoEvents.size());
    for(size_t i = 0; i < TempoEvents.size(); i++)
    {
        uint64_t segStart  = TempoEvents[i].tick;
        uint64_t segEnd    = (i + 1 < TempoEvents.size()) ? TempoEvents[i + 1].tick : UINT64_MAX;
        double   usPerQn   = TempoEvents[i].usPerQuarter;
        double   spb       = usPerQn * 1e-6 / ppnq;
        double   segStartT = TempoEvents[i].T;
        TempoCache.push_back({ segStart, segEnd, segStartT, usPerQn, spb });
    }
}

double NVnoteList::tickToSeconds(uint64_t tick) const
{
    if(TempoCache.empty()) return (double)tick * (0.5 / MIDI_File.ppnq);
    auto it = std::upper_bound(TempoCache.begin(), TempoCache.end(), tick,
        [](uint64_t t, const TempoSegment &seg) { return t < seg.endTick; });
    if(it == TempoCache.begin()) it++;
    it--;
    return it->startTime + (double)(tick - it->startTick) * it->secondsPerTick;
}

uint64_t NVnoteList::secondsToTick(double seconds) const
{
    if(TempoCache.empty()) return (uint64_t)(seconds / (0.5 / MIDI_File.ppnq));
    auto it = std::upper_bound(TempoCache.begin(), TempoCache.end(), seconds,
        [](double t, const TempoSegment &seg) { return t < seg.startTime; });
    if(it != TempoCache.begin()) --it;
    if(it->secondsPerTick <= 0) return it->startTick;
    return it->startTick + (uint64_t)((seconds - it->startTime) / it->secondsPerTick);
}

f64 NVnoteList::get_tempo_at_tick(uint64_t tick) const
{
    if(TempoCache.empty()) return 500000.0;
    auto it = std::upper_bound(TempoCache.begin(), TempoCache.end(), tick,
        [](uint64_t t, const TempoSegment &seg) { return t < seg.endTick; });
    if(it == TempoCache.begin()) it++;
    it--;
    return it->usPerQuarter;
}

void NVnoteList::destroy_all()
{
    delete[] keys;
    keys = nullptr;
    MIDI_File.mid_close();
    Evt_sequencer.seq_destroy();
    TempoEvents.clear();
    TempoEvents.shrink_to_fit();
    TempoCache.clear();
    TempoCache.shrink_to_fit();
}

void NVnoteList::list_seek(f64 T)
{
    if(T < Tread)
    {
        abstick = 0;
        Tread   = 0.0;
        dT      = 0.5 / MIDI_File.ppnq;
        MIDI_File.rewind_all();
        Evt_sequencer.seq_reset(MIDI_File);
        TempoEvents.clear();
        TempoEvents.push_back({ 0, 0.0, 500000.0 });
        TempoCache.clear();
    }

    for(int i = 0; i < 128; i++)
    {
        Note_list[i].clear();

        for(NVMidi::u16_t t = 0; t < MIDI_File.tracks; t++)
        {
            while(!keys[t][i].empty())
            {
                keys[t][i].pop();
            }
        }
    }

    while(Evt_sequencer.event().track < MIDI_File.tracks)
    {
        const NVseq_event &Evt  = Evt_sequencer.event();
        Tread                  += dT * (Evt.abstick - abstick);
        abstick                 = Evt.abstick;

        if(Tread >= T)
        {
            break;
        }

        if(Evt.type == NV_METYPE::META && Evt.num == 0x51u)
        {
            NVMidi::u32_t speed = Evt.data[0];
            speed               = speed << 8 | Evt.data[1];
            speed               = speed << 8 | Evt.data[2];
            dT                  = 0.000001 * speed / MIDI_File.ppnq;
            TempoEvents.push_back({ Evt.abstick, Tread, (f64)speed });
        }

        Evt_sequencer.seq_next(MIDI_File);
    }

    if(TempoEvents.size() > TempoCache.size())
        build_tempo_cache();
}

void NVnoteList::update_to(f64 T)
{
    while(Evt_sequencer.event().track < MIDI_File.tracks)
    {
        const NVseq_event &Evt  = Evt_sequencer.event();
        Tread                  += dT * (Evt.abstick - abstick);
        abstick                 = Evt.abstick;

        if(Tread >= T)
        {
            break;
        }

        switch(Evt.type)
        {
        case(NV_METYPE::META):
            if(Evt.num == 0x51u)
            {
                NVMidi::u32_t speed = Evt.data[0];
                speed               = speed << 8 | Evt.data[1];
                speed               = speed << 8 | Evt.data[2];
                dT                  = 0.000001 * speed / MIDI_File.ppnq;
                TempoEvents.push_back({ Evt.abstick, Tread, (f64)speed });
            }
            break;

        case(NV_METYPE::NOTE_ON):
            if(Evt.value > 0)
            {
                Note_list[Evt.num].emplace_back(Tread, Evt);
                auto nt = Note_list[Evt.num].end();
                keys[Evt.track][Evt.num].push(--nt);
                break;
            }
            break;
        case(NV_METYPE::NOTE_OFF):
            if(!keys[Evt.track][Evt.num].empty())
            {
                keys[Evt.track][Evt.num].top()->Tend = Tread;
                keys[Evt.track][Evt.num].pop();
            }
            break;
        default:
            break;
        }

        Evt_sequencer.seq_next(MIDI_File);
    }

    if(TempoEvents.size() > TempoCache.size())
        build_tempo_cache();
}

void NVnoteList::OR() // A presumably useful overlap remover
{
    for(int i = 0; i < 128; i++)
    {
        std::list<NVnote>::iterator p  = Note_list[i].end();
        f64                         T0 = 114514191981.0;
        f64                         T1 = 114514191981.0;

        while(p-- != Note_list[i].begin())
        {
            if(T0 < p->Tend && p->Tend < T1)
            {
                p->Tend = T0, T0 = p->Tstart;

                if(p->Tend - p->Tstart < 1e-7)
                {
                    p = Note_list[i].erase(p);
                }
            }
            else
            {
                T0 = p->Tstart, T1 = p->Tend;
            }
        }
    }
}

void NVnoteList::remove_to(f64 T)
{
    for(int i = 0; i < 128; i++)
    {
        std::list<NVnote>::iterator p = Note_list[i].begin();

        while(p != Note_list[i].end() && p->Tstart < T)
        {
            p->Tend < T ? (p = Note_list[i].erase(p)) : p++;
        }
    }
}
