#pragma once

#include <cstdio>
#include <string>

class VideoExporter
{
public:
    static bool start(int width, int height, int fps, const char* output_path, bool video = true);
    static void capture_audio();
    static void stop();
    static void abort();
    static bool is_recording();
    static int  get_frame_count();

private:
    static void ensure_dsp();

    static bool         s_recording;
    static bool         s_aborted;
    static int          s_frame_count;
    static int          s_audio_bytes;
    static FILE*        s_audio_file;
    static unsigned int s_dsp;
    static std::string  s_output_path;
    static std::string  s_temp_dir;
    static std::string  s_audio_path;
};
