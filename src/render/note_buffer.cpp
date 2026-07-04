#include <algorithm>
#include <map>
#include <random>
#include <unordered_map>



#include "../globals.h"
#include "note_buffer.h"
#include "render.h"














std::map<std::pair<int, int>, unsigned int> NoteBuffer::trackChannelColorMap;

unsigned int                                NoteBuffer::GenerateRandomColor()
{
    static std::mt19937                                rng(std::random_device {}());
    static std::uniform_int_distribution<unsigned int> dist(60, 255); // Ensure brightness by using a higher range

    // Generate bright red, green, and blue components
    unsigned int                                       r = dist(rng);
    unsigned int                                       g = dist(rng);
    unsigned int                                       b = dist(rng);

    // Combine the RGB components into a single color value
    return (r << 16) | (g << 8) | b;
}

void NoteBuffer::ClearTrackChannelColors()
{
    trackChannelColorMap.clear();
    std::map<std::pair<int, int>, unsigned int>().swap(trackChannelColorMap);
}

void NoteBuffer::DrawNotes(Config::configuration settings)
{
    std::map<NVMidi::u16_t, std::vector<const Note *>>           keyBuckets;
    static std::unordered_map<int, decltype(Render::note_color)> _randomColorCache;


    for(const auto &q : note_buf)
        keyBuckets[q.k].push_back(&q);

    std::vector<NVMidi::u16_t> keys;
    keys.reserve(keyBuckets.size());

    for(const auto &[k, _] : keyBuckets)
        keys.push_back(k);

    std::sort(keys.begin(), keys.end());

    for(auto k : keys)
    {
        auto &bucket = keyBuckets[k];

        // Note layering / sorting
        std::stable_sort(bucket.begin(), bucket.end(),
            [](const Note *a, const Note *b)
            {
                const auto &A      = a->n;
                const auto &B      = b->n;

                // White notes first then the black notes after
                bool        sharpA = Render::IsSharp(a->k);
                bool        sharpB = Render::IsSharp(b->k);
                if(sharpA != sharpB)
                    return !sharpA; // white first

                auto durA = A.Tend - A.Tstart;
                auto durB = B.Tend - B.Tstart;

                if(durA != durB)
                    return durA > durB; // Longer notes drawn first (underneath)

                if(A.track != B.track)
                    return A.track < B.track;

                // if(A.chn != B.chn)
                //     return A.chn < B.chn;
                //

                // Fallback: Earlier start time on top
                return A.Tstart > B.Tstart;
            });

        // Note drawing
        for(const auto *qptr : bucket)
        {
            const NVnote       &n               = qptr->n;
            NVMidi::u16_t       k               = qptr->k;
            int                 pps             = qptr->pps;

            // Note color distribution
            std::pair<int, int> trackChannelKey = { n.track, n.chn };

            auto                it              = trackChannelColorMap.find(trackChannelKey);
            int                 colorIndex;

            if(it == trackChannelColorMap.end())
            {
                colorIndex                            = trackChannelColorMap.size();
                trackChannelColorMap[trackChannelKey] = colorIndex;
            }
            else
                colorIndex = it->second;

            if(!settings.is_custom_ch_colors)
            {
                if(settings.loop_colors)
                    Render::note_color = settings.channel_colors[colorIndex % 16];
                else
                {
                    if(colorIndex < 16)
                        Render::note_color = default_settings.channel_colors[colorIndex];
                    else
                    {
                        // Render::note_color = GenerateRandomColor();

                        auto [it, inserted] = _randomColorCache.try_emplace(colorIndex);
                        if(inserted)
                            it->second = GenerateRandomColor();
                        Render::note_color = it->second;
                    }
                }
            }
            else
            {
                if(settings.loop_colors)
                    Render::note_color = settings.channel_colors[colorIndex % 16];
                else
                {
                    if(colorIndex < 16)
                        Render::note_color = settings.channel_colors[colorIndex];
                    else
                    {
                        // Render::note_color = GenerateRandomColor();

                        auto [it, inserted] = _randomColorCache.try_emplace(colorIndex);
                        if(inserted)
                            it->second = GenerateRandomColor();
                        Render::note_color = it->second;
                    }
                }
            }

            int key = Render::KeyMap[k];

            // Gradient rect (Note fill)
            int y_0 = std::clamp((int)floor(_WinH - (n.Tstart - Playback::Tplay) * pps + 0.5f), 0, _WinH);

            // Dark rect (Note outline)
            int y_1 = (n.Tend < Playback::Tplay + Tscr) ? std::clamp((int)floor(_WinH - (n.Tend - Playback::Tplay) * pps + 0.5f), 0, _WinH) : 0;

            if(n.Tstart <= Playback::Tplay && Playback::Tplay < n.Tend)
            {
                RenderWin->KeyPress[key] = true;
                RenderWin->KeyColor[key] = Render::note_color;
            }

            RenderWin->CreateNote(k, y_0, y_1, Render::note_color);
        }
    }

    note_buf.clear();
}