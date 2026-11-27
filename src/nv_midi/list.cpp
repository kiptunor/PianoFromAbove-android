#include <algorithm>
#include <unordered_map>
#include <cmath>
#include <array>
#include <vector>

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

void NVnoteList::collect_all_notes(std::vector<GpuNote>& out)
{
    out.clear();
    out.reserve(1 << 20);

    // Reset parser/sequencer/tempo state to the beginning of the MIDI.
    list_seek(0.0);

    const u32 NONE = 0xFFFFFFFFu;
    const size_t slotCount = static_cast<size_t>(MIDI_File.tracks) * 128u;
    std::vector<u32> openSlot(slotCount, NONE);
    std::unordered_map<u32, std::vector<u32>> openOverflow;

    std::vector<f64> startTimes;
    startTimes.reserve(1 << 20);

    // Stable track/channel -> color index mapping, same idea as NoteBuffer.
    std::unordered_map<u32, u16> colorMap;
    u16 nextColor = 0;
    auto colorIndexFor = [&](u16 track, u8 chn) -> u8
    {
        u32 k = (u32(track) << 8) | u32(chn);
        auto [it, inserted] = colorMap.emplace(k, nextColor);

        if(inserted)
            ++nextColor;

        return static_cast<u8>(it->second % 256u);
    };

    auto finalize = [&](u32 idx, f64 endT)
    {
        if(idx >= out.size() || idx >= startTimes.size())
            return;

        const f64 startD = startTimes[idx];
        f64 lenD = endT - startD;

        if(!std::isfinite(lenD) || lenD <= 0.0)
            lenD = 0.0001;

        out[idx].length = static_cast<f32>(lenD);
    };

    // Walk all sequencer events until EOF.
    while(Evt_sequencer.event().track < MIDI_File.tracks)
    {
        const NVseq_event &Evt = Evt_sequencer.event();
        Tread  += dT * static_cast<f64>(Evt.abstick - abstick);
        abstick = Evt.abstick;

        switch(Evt.type)
        {
            case NV_METYPE::META:
                if(Evt.num == 0x51u) // tempo change
                {
                    NVMidi::u32_t speed = Evt.data[0];
                    speed = (speed << 8) | Evt.data[1];
                    speed = (speed << 8) | Evt.data[2];
                    dT = 0.000001 * static_cast<f64>(speed) / MIDI_File.ppnq;
                    TempoEvents.push_back({Evt.abstick, Tread, static_cast<f64>(speed)});
                }
                break;

            case NV_METYPE::NOTE_ON:
                if(Evt.value > 0)
                {
                    if(Evt.num >= 128u || Evt.track >= MIDI_File.tracks)
                        break;

                    const u32 slot = static_cast<u32>(Evt.track) * 128u + static_cast<u32>(Evt.num);
                    const u32 idx  = static_cast<u32>(out.size());
                    const u8 colorIdx = colorIndexFor(Evt.track, Evt.chan);

                    GpuNote gn{};
                    gn.start           = static_cast<f32>(Tread);
                    gn.length          = 0.0f; // finalized on note-off/end
                    gn.key             = static_cast<u32>(Evt.num);
                    gn.colorIndexFlags = (static_cast<u32>(Evt.track) << 8) | static_cast<u32>(colorIdx);

                    if(openSlot[slot] != NONE)
                        openOverflow[slot].push_back(openSlot[slot]);

                    openSlot[slot] = idx;
                    startTimes.push_back(Tread);
                    out.push_back(gn);
                    break;
                }
                // velocity 0 == note-off, fall through
                [[fallthrough]];

            case NV_METYPE::NOTE_OFF:
            {
                if(Evt.num >= 128u || Evt.track >= MIDI_File.tracks)
                    break;

                const u32 slot = static_cast<u32>(Evt.track) * 128u + static_cast<u32>(Evt.num);
                if(openSlot[slot] != NONE)
                {
                    finalize(openSlot[slot], Tread);
                    auto it = openOverflow.find(slot);
                    if(it != openOverflow.end() && !it->second.empty())
                    {
                        openSlot[slot] = it->second.back();
                        it->second.pop_back();
                        if(it->second.empty())
                            openOverflow.erase(it);
                    }
                    else
                        openSlot[slot] = NONE;
                    
                }
                break;
            }

            default:
                break;
        }

        Evt_sequencer.seq_next(MIDI_File);
    }

    // Finalize notes that never received a note-off.
    for(size_t slot = 0; slot < slotCount; ++slot)
    {
        auto it = openOverflow.find(static_cast<u32>(slot));
        if(it != openOverflow.end())
        {
            for(u32 idx : it->second)
                finalize(idx, Tread);

            openOverflow.erase(it);
        }

        if(openSlot[slot] != NONE)
        {
            finalize(openSlot[slot], Tread);
            openSlot[slot] = NONE;
        }
    }

    // startTimes no longer needed after finalization
    std::vector<f64>().swap(startTimes);

    if(TempoEvents.size() > TempoCache.size())
        build_tempo_cache();

    // Match SDLRenderer/note_buffer.cpp layering:
    // bucket by key in ascending key order, then within key:
    // white first/black after, longer duration first (underneath),
    // lower track first, earlier start on top.
    std::stable_sort(out.begin(), out.end(), [](const GpuNote& a, const GpuNote& b)
    {
        auto isSharp = [](u32 k)
        {
            int n = static_cast<int>(k % 12);
            return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
        };
        bool sharpA = isSharp(a.key);
        bool sharpB = isSharp(b.key);
        
        if(sharpA != sharpB)
            return !sharpA; // white first

        if(a.start != b.start)
            return a.start < b.start;

        if(a.length != b.length)
            return a.length > b.length; // longer underneath

        u32 trackA = a.colorIndexFlags >> 8;
        u32 trackB = b.colorIndexFlags >> 8;
        
        if(trackA != trackB)
            return trackA < trackB;

        return a.key < b.key;
    });
}