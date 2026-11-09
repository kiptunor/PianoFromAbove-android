#ifndef AUDIO_ENC_H
#define AUDIO_ENC_H



#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
}


class AudioEncoder
{
  public:
    AudioEncoder();
    ~AudioEncoder();

    AudioEncoder(const AudioEncoder &)            = delete;
    AudioEncoder &operator=(const AudioEncoder &) = delete;

    bool          open(const std::string &filename, int sampleRate, int channels = 2);

    // samples: interleaved float PCM
    // frames: number of audio frames per channel
    bool          write(const float *samples, int frames);
    bool          close();

  private:
    bool             receivePackets();

    AVFormatContext *formatCtx     = nullptr;
    AVCodecContext  *codecCtx      = nullptr;
    AVStream        *stream        = nullptr;
    AVFrame         *frame         = nullptr;
    AVPacket        *packet        = nullptr;

    int              sampleRate    = 44100;
    int              channels      = 2;

    int64_t          nextPts       = 0;

    bool             headerWritten = false;
};
#endif