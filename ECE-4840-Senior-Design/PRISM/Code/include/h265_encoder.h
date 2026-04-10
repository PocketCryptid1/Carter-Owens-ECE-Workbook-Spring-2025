#ifndef H265_ENCODER_H
#define H265_ENCODER_H

#include <cstdint>
#include <cstdio>

// Forward declarations for FFmpeg types
struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct AVFormatContext;
struct AVStream;

// H265Encoder encodes YUV420 frames to H.265 using FFmpeg
// Outputs to an MP4 file that can be played directly
class H265Encoder {
public:
    H265Encoder();
    ~H265Encoder();

    // Initialize the encoder with specified resolution
    bool init(uint32_t width, uint32_t height, uint32_t bitrate = 5000000, uint32_t framerate = 30);

    // Encode a YUV420 frame
    bool encodeFrame(const void *frameData, size_t frameSize);

    // Finalize encoding and close file
    void finalize();

private:
    void cleanup();
    void writePacket(AVPacket *pkt);

    AVCodecContext *codecCtx;
    AVFrame *frame;
    AVPacket *pkt;
    AVFormatContext *formatCtx;
    AVStream *stream;
    
    uint32_t width, height, framerate;
    int64_t frameCount;
};

#endif // H265_ENCODER_H
