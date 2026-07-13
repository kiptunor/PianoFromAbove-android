#include "exporter.h"
#include "../logger.h"
#include "../mb_types.h"
#include "../audio/playback.h"
#include <bass.h>
#include <cstdlib>
#include <filesystem>
#include <thread>

// -------------------------------------------------------------------
// Static members
// -------------------------------------------------------------------
bool         VideoExporter::s_recording    = false;
bool         VideoExporter::s_aborted      = false;
int          VideoExporter::s_frame_count  = 0;
int          VideoExporter::s_audio_bytes  = 0;
FILE*        VideoExporter::s_audio_file   = nullptr;
unsigned int VideoExporter::s_dsp          = 0;
std::string  VideoExporter::s_output_path;
std::string  VideoExporter::s_temp_dir;
std::string  VideoExporter::s_audio_path;

// -------------------------------------------------------------------
// DSP callback -- called by BASS on its audio thread with raw PCM data
// user parameter holds the FILE* for the audio output
static void CALLBACK export_dsp(u32 handle, u32 channel, void *buffer, u32 length, void *user)
{
    (void)handle;
    (void)channel;
    FILE *f = (FILE *)user;
    if(VideoExporter::is_recording() && f)
        fwrite(buffer, 1, length, f);
}

// -------------------------------------------------------------------
bool VideoExporter::start(int width, int height, int fps, const char *output_path, bool /*video*/)
{
    if(s_recording)
        return false;

    // Create a temporry directory for raw data
    char tmp_template[] = "/tmp/pfa_audio_XXXXXX";
    char *tmp_dir = mkdtemp(tmp_template);
    if(!tmp_dir)
        return false;

    s_temp_dir    = tmp_dir;
    s_audio_path  = s_temp_dir + "/audio.raw";

    s_audio_file  = fopen(s_audio_path.c_str(), "wb");
    if(!s_audio_file)
    {
        std::filesystem::remove_all(s_temp_dir);
        return false;
    }

    s_frame_count = 0;
    s_audio_bytes = 0;
    s_dsp         = 0;
    s_output_path = output_path;
    s_recording   = true;
    s_aborted     = false;
    return true;
}

// -------------------------------------------------------------------
void VideoExporter::ensure_dsp()
{
    if(s_dsp || !Playback::main_stream || !s_audio_file)
        return;
    s_dsp = BASS_ChannelSetDSP(Playback::main_stream, &export_dsp, s_audio_file, 0);
}

// -------------------------------------------------------------------
void VideoExporter::capture_audio()
{
    if(!s_recording || !s_audio_file)
        return;
    ensure_dsp();
}

// -------------------------------------------------------------------
void VideoExporter::stop()
{
    if(!s_recording)
        return;
    s_recording = false;

    // Remove DSP before closing the file so no callback writes to a closed handle
    if(s_dsp && Playback::main_stream)
    {
        BASS_ChannelRemoveDSP(Playback::main_stream, s_dsp);
        s_dsp = 0;
    }

    // Close temp file
    if(s_audio_file) { fclose(s_audio_file); s_audio_file = nullptr; }

    if(s_aborted)
    {
        std::filesystem::remove_all(s_temp_dir);
        return;
    }

    // Launch ffmpeg in a background thread to avoid blocking the UI
    std::thread([]()
    {
        char cmd[4096];
        snprintf(cmd, sizeof(cmd),
            "ffmpeg -y -f f32le -ar 44100 -ac 2 -i \"%s\" \"%s\" 2>>/tmp/pfa_ffmpeg.log",
            s_audio_path.c_str(),
            s_output_path.c_str());

        Log::debug("Export ffmpeg: %s", cmd);
        int ret = system(cmd);
        Log::debug("Export ffmpeg exit code: %d", ret);

        // Clean up temp files
        std::filesystem::remove_all(s_temp_dir);

        // Reset state
        s_frame_count = 0;
        s_audio_bytes = 0;
    }).detach();
}

// -------------------------------------------------------------------
void VideoExporter::abort()
{
    s_aborted = true;
    stop();
}

// -------------------------------------------------------------------
bool VideoExporter::is_recording()
{
    return s_recording;
}

// -------------------------------------------------------------------
int VideoExporter::get_frame_count()
{
    return s_frame_count;
}
