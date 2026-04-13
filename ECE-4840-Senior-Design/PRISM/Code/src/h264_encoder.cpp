#include "h264_encoder.h"
#include <iostream>
#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

using namespace std;

H264Encoder::H264Encoder() : codecCtx(nullptr), frame(nullptr), pkt(nullptr), 
                             formatCtx(nullptr), stream(nullptr), frameCount(0) {}

H264Encoder::~H264Encoder() {
    cleanup();
}

bool H264Encoder::init(uint32_t width, uint32_t height, uint32_t bitrate, uint32_t framerate) {
    this->width = width;
    this->height = height;
    this->framerate = framerate;

    // Find hardware encoder on Raspberry Pi
    // RPi only supports H.264 hardware encoding
    const AVCodec *codec = avcodec_find_encoder_by_name("h264_v4l2m2m");
    if (!codec) {
        cerr << "Hardware H.264 encoder not found, falling back to software encoder...\n";
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    }
    if (!codec) {
        cerr << "No H.264 encoder found\n";
        return false;
    }

    // Create codec context
    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        cerr << "Failed to allocate codec context\n";
        return false;
    }

    // Configure encoder
    codecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    codecCtx->width = width;
    codecCtx->height = height;
    codecCtx->bit_rate = bitrate;
    codecCtx->time_base = {1, (int)framerate};
    codecCtx->framerate = {(int)framerate, 1};
    codecCtx->gop_size = 10;
    codecCtx->max_b_frames = 0;

    // Open encoder
    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        cerr << "Failed to open encoder\n";
        avcodec_free_context(&codecCtx);
        return false;
    }

    // Allocate frame
    frame = av_frame_alloc();
    if (!frame) {
        cerr << "Failed to allocate frame\n";
        return false;
    }

    frame->format = codecCtx->pix_fmt;
    frame->width = codecCtx->width;
    frame->height = codecCtx->height;

    if (av_frame_get_buffer(frame, 0) < 0) {
        cerr << "Failed to allocate frame buffer\n";
        av_frame_free(&frame);
        return false;
    }

    // Allocate packet
    pkt = av_packet_alloc();
    if (!pkt) {
        cerr << "Failed to allocate packet\n";
        return false;
    }

    // Create output format context
    avformat_alloc_output_context2(&formatCtx, nullptr, "mp4", "encoded_output.mp4");
    if (!formatCtx) {
        cerr << "Failed to create output context\n";
        return false;
    }

    // Create output stream
    stream = avformat_new_stream(formatCtx, codec);
    if (!stream) {
        cerr << "Failed to create stream\n";
        return false;
    }

    stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    stream->codecpar->codec_id = AV_CODEC_ID_H264;
    stream->codecpar->width = width;
    stream->codecpar->height = height;
    stream->codecpar->bit_rate = bitrate;
    stream->time_base = {1, (int)framerate};

    avcodec_parameters_from_context(stream->codecpar, codecCtx);

    // Open output file
    if (!(formatCtx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&formatCtx->pb, "encoded_output.mp4", AVIO_FLAG_WRITE) < 0) {
            cerr << "Failed to open output file\n";
            return false;
        }
    }

    // Write header
    if (avformat_write_header(formatCtx, nullptr) < 0) {
        cerr << "Failed to write header\n";
        return false;
    }

    cout << "H.264 encoder initialized: " << width << "x" << height 
         << " @ " << bitrate << " bps, " << framerate << " fps\n";
    cout << "Output file: encoded_output.mp4\n";
    
    return true;
}

bool H264Encoder::encodeFrame(const void *frameData, size_t frameSize) {
    if (!codecCtx || !frame || !formatCtx) {
        cerr << "Encoder not initialized\n";
        return false;
    }

    // Verify frame size
    size_t expectedSize = width * height * 3 / 2;
    if (frameSize != expectedSize) {
        cerr << "Frame size mismatch: got " << frameSize << ", expected " << expectedSize << "\n";
        return false;
    }

    // Copy YUV420 data into frame
    uint8_t *src = (uint8_t*)frameData;
    
    // Y plane
    memcpy(frame->data[0], src, width * height);
    src += width * height;
    
    // U plane
    memcpy(frame->data[1], src, width * height / 4);
    src += width * height / 4;
    
    // V plane
    memcpy(frame->data[2], src, width * height / 4);

    frame->pts = frameCount++;

    // Send frame to encoder
    if (avcodec_send_frame(codecCtx, frame) < 0) {
        cerr << "Failed to send frame to encoder\n";
        return false;
    }

    // Receive and write encoded packets
    int ret = 0;
    while (ret >= 0) {
        ret = avcodec_receive_packet(codecCtx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            cerr << "Error receiving packet from encoder\n";
            return false;
        }

        writePacket(pkt);
    }

    return true;
}

void H264Encoder::finalize() {
    if (!codecCtx || !formatCtx) {
        return;
    }

    // Flush encoder
    avcodec_send_frame(codecCtx, nullptr);
    int ret = 0;
    while (ret >= 0) {
        ret = avcodec_receive_packet(codecCtx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret >= 0) {
            writePacket(pkt);
        }
    }

    // Write trailer
    av_write_trailer(formatCtx);
    
    cout << "Encoded " << frameCount << " frames to encoded_output.mp4\n";
}

void H264Encoder::writePacket(AVPacket *pkt) {
    if (!pkt || !stream || !formatCtx) {
        return;
    }

    av_packet_rescale_ts(pkt, codecCtx->time_base, stream->time_base);
    pkt->stream_index = stream->index;

    if (av_interleaved_write_frame(formatCtx, pkt) < 0) {
        cerr << "Error writing frame\n";
    }

    av_packet_unref(pkt);
}

void H264Encoder::cleanup() {
    if (formatCtx) {
        if (!(formatCtx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&formatCtx->pb);
        }
        avformat_free_context(formatCtx);
        formatCtx = nullptr;
    }

    if (codecCtx) {
        avcodec_free_context(&codecCtx);
        codecCtx = nullptr;
    }

    if (frame) {
        av_frame_free(&frame);
        frame = nullptr;
    }

    if (pkt) {
        av_packet_free(&pkt);
        pkt = nullptr;
    }

    stream = nullptr;
}
