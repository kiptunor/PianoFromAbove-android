#include "exporter.h"
#include "../logger.h"
#include "../mb_types.h"
#include "../audio/playback.h"
#include <bass.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <thread>

#ifdef PLATFORM_ANDROID
#include <SDL3/SDL.h>
#include <sys/stat.h>
#include <unistd.h>

// Extract ffmpeg binary from APK assets to a writable + executable location
static std::string get_ffmpeg_path()
{
    const char *cache = SDL_GetAndroidCachePath();
    if(!cache) return "";

    std::string path = std::string(cache) + "/ffmpeg";

    struct stat st;
    if(stat(path.c_str(), &st) == 0 && (st.st_mode & S_IXUSR))
        return path;

    SDL_IOStream *rw = SDL_IOFromFile("ffmpeg_android", "rb");
    if(!rw)
    {
        Log::debug("ffmpeg binary not found in APK assets");
        return "";
    }

    FILE *f = fopen(path.c_str(), "wb");
    if(!f) { SDL_CloseIO(rw); return ""; }

    char buf[8192];
    size_t n;
    while((n = SDL_ReadIO(rw, buf, sizeof(buf))) > 0)
    {
        if(fwrite(buf, 1, n, f) != n)
        {
            fclose(f);
            SDL_CloseIO(rw);
            unlink(path.c_str());
            return "";
        }
    }

    fclose(f);
    SDL_CloseIO(rw);

    if(chmod(path.c_str(), 0755) != 0)
    {
        unlink(path.c_str());
        return "";
    }

    Log::debug("Extracted ffmpeg to %s", path.c_str());
    return path;
}
#endif
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
// Write a WAV file from raw f32le PCM data (works everywhere, no ffmpeg)
static bool write_wav(const char *raw_path, const char *wav_path)
{
    FILE *raw = fopen(raw_path, "rb");
    if(!raw) return false;

    fseek(raw, 0, SEEK_END);
    long data_bytes = ftell(raw);
    fseek(raw, 0, SEEK_SET);

    FILE *wav = fopen(wav_path, "wb");
    if(!wav) { fclose(raw); return false; }

    u32 sample_rate = 44100;
    u16 channels    = 2;
    u16 bits_sample = 32;
    u16 format_tag  = 3; // WAVE_FORMAT_IEEE_FLOAT
    u32 byte_rate   = sample_rate * channels * (bits_sample / 8);
    u16 block_align = channels * (bits_sample / 8);
    u32 data_size   = (u32)data_bytes;
    u32 file_size   = 36 + data_size;

    // RIFF header
    fwrite("RIFF", 1, 4, wav);
    fwrite(&file_size, 4, 1, wav);
    fwrite("WAVE", 1, 4, wav);

    // fmt subchunk
    fwrite("fmt ", 1, 4, wav);
    u32 fmt_size = 16;
    fwrite(&fmt_size, 4, 1, wav);
    fwrite(&format_tag, 2, 1, wav);
    fwrite(&channels, 2, 1, wav);
    fwrite(&sample_rate, 4, 1, wav);
    fwrite(&byte_rate, 4, 1, wav);
    fwrite(&block_align, 2, 1, wav);
    fwrite(&bits_sample, 2, 1, wav);

    // data subchunk
    fwrite("data", 1, 4, wav);
    fwrite(&data_size, 4, 1, wav);

    // PCM data
    char buf[65536];
    long remaining = data_bytes;
    while(remaining > 0)
    {
        long chunk = remaining < (long)sizeof(buf) ? remaining : (long)sizeof(buf);
        if(fread(buf, 1, (size_t)chunk, raw) != (size_t)chunk) break;
        if(fwrite(buf, 1, (size_t)chunk, wav) != (size_t)chunk) break;
        remaining -= chunk;
    }

    fclose(raw);
    fclose(wav);
    return remaining == 0;
}

// -------------------------------------------------------------------
bool VideoExporter::start(int width, int height, int fps, const char *output_path, bool /*video*/)
{
    if(s_recording)
        return false;

    // Create a temporary directory for raw data
    char tmp_template[512];
#ifdef PLATFORM_ANDROID
    const char *cache = SDL_GetAndroidCachePath();
    if(cache)
        snprintf(tmp_template, sizeof(tmp_template), "%s/pfa_audio_XXXXXX", cache);
    else
        snprintf(tmp_template, sizeof(tmp_template), "/data/local/tmp/pfa_audio_XXXXXX");
#else
    snprintf(tmp_template, sizeof(tmp_template), "/tmp/pfa_audio_XXXXXX");
#endif
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

    // Launch export in a background thread to avoid blocking the UI
    std::thread([]()
    {
        std::string ext;
        size_t dot = s_output_path.rfind('.');
        if(dot != std::string::npos)
            ext = s_output_path.substr(dot);

        if(ext == ".wav")
        {
            if(write_wav(s_audio_path.c_str(), s_output_path.c_str()))
                Log::debug("Export WAV: %s", s_output_path.c_str());
            else
                Log::debug("Export WAV FAILED: %s", s_output_path.c_str());
        }
        else
        {
            char cmd[4096];
#ifdef PLATFORM_ANDROID
            std::string ffmpeg = get_ffmpeg_path();
            if(ffmpeg.empty())
            {
                Log::debug("Export FAILED: ffmpeg binary not available on this device");
            }
            else
            {
                const char *cache = SDL_GetAndroidCachePath();
                std::string log = std::string(cache ? cache : ".") + "/pfa_ffmpeg.log";
                snprintf(cmd, sizeof(cmd),
                    "\"%s\" -y -f f32le -ar 44100 -ac 2 -i \"%s\" \"%s\" 2>>\"%s\"",
                    ffmpeg.c_str(),
                    s_audio_path.c_str(),
                    s_output_path.c_str(),
                    log.c_str());
                Log::debug("Export ffmpeg: %s", cmd);
                int ret = system(cmd);
                Log::debug("Export ffmpeg exit code: %d", ret);
            }
#else
            snprintf(cmd, sizeof(cmd),
                "ffmpeg -y -f f32le -ar 44100 -ac 2 -i \"%s\" \"%s\" 2>>/tmp/pfa_ffmpeg.log",
                s_audio_path.c_str(),
                s_output_path.c_str());

            Log::debug("Export ffmpeg: %s", cmd);
            int ret = system(cmd);
            Log::debug("Export ffmpeg exit code: %d", ret);
#endif
        }

        std::filesystem::remove_all(s_temp_dir);
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
