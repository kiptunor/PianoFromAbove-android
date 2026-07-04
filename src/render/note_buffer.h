#ifndef NOTE_BUFFER_H
#define NOTE_BUFFER_H

#include "../config/config.h"
#include "../nv_midi/list.h"

#include <map>














class NoteBuffer
{
  public:
    struct Note
    {
        NVMidi::u16_t k;
        NVnote        n;
        int           pps;
    };

    static std::map<std::pair<int, int>, unsigned int> trackChannelColorMap;

    static void                                        DrawNotes(Config::configuration settings);
    static void                                        ClearTrackChannelColors();
    static unsigned int                                GenerateRandomColor();
};

#endif // NOTE_BUFFER_H
