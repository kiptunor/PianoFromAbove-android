#include "audio_enc.h"
#include "../logger.h"



AudioEncoder::AudioEncoder() = default;

AudioEncoder::~AudioEncoder()
{
    close();
}

bool AudioEncoder::open(const std::string &filename, int sampleRate, int channels)
{
    if(formatCtx || codecCtx)
        close();

    this->sampleRate = sampleRate;
    this->channels   = channels;

    if(sampleRate <= 0 || channels <= 0)
    {
        Log::error("Invalid sample rate or channel count");
        return 0;
    }

    int ret = avformat_alloc_output_context2(&formatCtx, nullptr, "wav", filename.c_str());

    if(ret < 0 || !formatCtx)
    {
        Log::error("Could not create output context");
        return 0;
    }

    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);

    if(!codec)
    {
        Log::error("PCM S16LE encoder not found");
        return 0;
    }

    stream = avformat_new_stream(formatCtx, nullptr);

    if(!stream)
    {
        Log::error("Could not create audio stream");
        return 0;
    }

    codecCtx = avcodec_alloc_context3(codec);

    if(!codecCtx)
    {
        Log::error("Could not allocate codec context");
        return 0;
    }

    codecCtx->sample_rate = sampleRate;
    codecCtx->sample_fmt  = AV_SAMPLE_FMT_S16;
    codecCtx->time_base   = AVRational { 1, sampleRate };
    codecCtx->bit_rate    = sampleRate * channels * 16;

    av_channel_layout_default(&codecCtx->ch_layout, channels);

    stream->time_base = codecCtx->time_base;

    ret               = avcodec_open2(codecCtx, codec, nullptr);

    if(ret < 0)
    {
        Log::error("Could not open encoder");
        return 0;
    }

    ret = avcodec_parameters_from_context(stream->codecpar, codecCtx);

    if(ret < 0)
    {
        Log::error("Could not copy codec parameters");
        return 0;
    }

    if(!(formatCtx->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&formatCtx->pb, filename.c_str(), AVIO_FLAG_WRITE);

        if(ret < 0)
        {
            Log::error("Could not open output file");
            return 0;
        }
    }

    ret = avformat_write_header(formatCtx, nullptr);

    if(ret < 0)
    {
        Log::error("Could not write file header");
        return 0;
    }

    headerWritten = true;

    frame         = av_frame_alloc();
    packet        = av_packet_alloc();

    if(!frame || !packet)
    {
        Log::error("Could not allocate frame or packet");
        return 0;
    }

    frame->format      = codecCtx->sample_fmt;
    frame->sample_rate = sampleRate;
    frame->nb_samples  = 1024;

    av_channel_layout_copy(&frame->ch_layout, &codecCtx->ch_layout);

    ret = av_frame_get_buffer(frame, 0);

    if(ret < 0)
    {
        Log::error("Could not allocate audio buffer");
        return 0;
    }

    nextPts = 0;

    return true;
}

bool AudioEncoder::write(const float *samples, int frames)
{
    if(!codecCtx || !samples || frames <= 0)
        return false;

    constexpr int encoderFrameSize = 1024;

    int           offset           = 0;

    while(offset < frames)
    {
        int count         = std::min(encoderFrameSize, frames - offset);
        frame->nb_samples = encoderFrameSize;
        int ret           = av_frame_make_writable(frame);

        if(ret < 0)
        {
            Log::error("Failed to write audio frame");
            return 0;
        }

        frame->nb_samples = count;
        frame->pts        = nextPts;

        auto *dst         = reinterpret_cast<int16_t *>(frame->data[0]);
        int   sampleCount = count * channels;

        for(int i = 0; i < sampleCount; ++i)
        {
            float value = samples[offset * channels + i];
            value      = std::clamp(value, -1.0f, 1.0f);
            dst[i]     = static_cast<int16_t>(std::lrint(value * 32767.0f));
        }

        nextPts += count;
        ret      = avcodec_send_frame(codecCtx, frame);

        if(ret < 0)
        {
            Log::error("Failed to send audio frame");
            return 0;
        }

        if(!receivePackets())
            return false;

        offset += count;
    }

    return true;
}

bool AudioEncoder::receivePackets()
{
    while(true)
    {
        int ret = avcodec_receive_packet(codecCtx, packet);

        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return true;

        if(ret < 0)
        {
            Log::error("Failed to encode audio");
            return 0;
        }

        av_packet_rescale_ts(packet, codecCtx->time_base, stream->time_base);

        packet->stream_index = stream->index;
        ret                  = av_interleaved_write_frame(formatCtx, packet);
        av_packet_unref(packet);

        if(ret < 0)
        {
            Log::error("Failed to write audio packet");
            return 0;
        }
    }
}

bool AudioEncoder::close()
{
    if(!formatCtx)
        return true;

    bool success = true;

    if(codecCtx && headerWritten)
    {
        int ret = avcodec_send_frame(codecCtx, nullptr);

        if(ret >= 0)
        {
            if(!receivePackets())
                success = false;
        }
        else
            success = false;
        
    }

    if(headerWritten)
    {
        int ret = av_write_trailer(formatCtx);

        if(ret < 0)
        {
            Log::error("Could not write file trailer");
            success = false;
        }

        headerWritten = false;
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&codecCtx);

    if(formatCtx->pb)
        avio_closep(&formatCtx->pb);

    avformat_free_context(formatCtx);

    formatCtx = nullptr;
    stream    = nullptr;

    nextPts   = 0;

    return success;
}