#include "display.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <jpeglib.h>
#include <csetjmp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

static AVCodecContext* g_h264CodecCtx = nullptr;

static const char* FB_DEV = "/dev/fb0";

struct JpegErrorManager {
    jpeg_error_mgr pub;
    jmp_buf setjmpBuffer;
};

extern "C" void jpegErrorExit(j_common_ptr cinfo) {
    JpegErrorManager* err = reinterpret_cast<JpegErrorManager*>(cinfo->err);
    longjmp(err->setjmpBuffer, 1);
}

static bool blitRGBToFramebuffer(const uint8_t* rgb, uint32_t imgWidth, uint32_t imgHeight) {
    // Persistent framebuffer — opened once for the process lifetime
    static int fb_fd = -1;
    static uint8_t* fb = nullptr;
    static size_t fb_size = 0;
    static uint32_t fb_width = 0, fb_height = 0, fb_bpp = 0, line_length = 0;

    if (fb_fd < 0) {
        fb_fd = open(FB_DEV, O_RDWR);
        if (fb_fd < 0) {
            std::cerr << "display: cannot open " << FB_DEV << " (run as root?)\n";
            return false;
        }
        struct fb_var_screeninfo vinfo;
        struct fb_fix_screeninfo finfo;
        if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) == -1 ||
            ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
            std::cerr << "display: failed to query framebuffer info\n";
            close(fb_fd); fb_fd = -1;
            return false;
        }
        fb_width    = vinfo.xres;
        fb_height   = vinfo.yres;
        fb_bpp      = vinfo.bits_per_pixel;
        fb_size     = finfo.smem_len;
        line_length = finfo.line_length;
        fb = static_cast<uint8_t*>(mmap(nullptr, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0));
        if (fb == MAP_FAILED) {
            std::cerr << "display: mmap failed\n";
            close(fb_fd); fb_fd = -1; fb = nullptr;
            return false;
        }
    }

    if (!rgb || imgWidth == 0 || imgHeight == 0) {
        return false;
    }

    uint32_t bytesPerPixel = fb_bpp / 8;

    for (uint32_t y = 0; y < fb_height; y++) {
        uint32_t srcY = (imgHeight > 0) ? ((y * imgHeight) / fb_height) : 0;
        if (srcY >= imgHeight) srcY = imgHeight - 1;
        const uint8_t* srcRow = rgb + static_cast<size_t>(srcY) * imgWidth * 3;
        uint8_t* dst = fb + y * line_length;

        for (uint32_t x = 0; x < fb_width; x++) {
            uint32_t srcX = (imgWidth > 0) ? ((x * imgWidth) / fb_width) : 0;
            if (srcX >= imgWidth) srcX = imgWidth - 1;
            const uint8_t* src = srcRow + srcX * 3;
            uint8_t r = src[0];
            uint8_t g = src[1];
            uint8_t b = src[2];

            if (bytesPerPixel == 4) {
                dst[x * 4 + 0] = b;
                dst[x * 4 + 1] = g;
                dst[x * 4 + 2] = r;
                dst[x * 4 + 3] = 0xff;
            } else if (bytesPerPixel == 3) {
                dst[x * 3 + 0] = b;
                dst[x * 3 + 1] = g;
                dst[x * 3 + 2] = r;
            } else if (bytesPerPixel == 2) {
                uint16_t pixel = static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
                dst[x * 2 + 0] = static_cast<uint8_t>(pixel & 0xFF);
                dst[x * 2 + 1] = static_cast<uint8_t>(pixel >> 8);
            }
        }
    }

    return true;
}

// Read a little-endian 32-bit int from a byte buffer
static uint32_t read32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t read16(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

bool displayBMP(const std::string& bmpPath) {
    // --- Load BMP file ---
    std::ifstream f(bmpPath, std::ios::binary);
    if (!f) {
        std::cerr << "display: cannot open " << bmpPath << "\n";
        return false;
    }
    std::vector<uint8_t> bmp((std::istreambuf_iterator<char>(f)), {});

    if (bmp.size() < 54 || bmp[0] != 'B' || bmp[1] != 'M') {
        std::cerr << "display: not a valid BMP file\n";
        return false;
    }

    uint32_t pixelOffset = read32(&bmp[10]);
    uint32_t imgWidth    = read32(&bmp[18]);
    uint32_t imgHeight   = read32(&bmp[22]);
    uint16_t bpp         = read16(&bmp[28]);

    if (bpp != 24) {
        std::cerr << "display: only 24-bit BMP supported (got " << bpp << "bpp)\n";
        return false;
    }

    // BMP rows are bottom-up and padded to 4-byte boundaries
    uint32_t rowSize = ((imgWidth * 3 + 3) / 4) * 4;

    std::vector<uint8_t> rgb(imgWidth * imgHeight * 3);

    for (uint32_t y = 0; y < imgHeight; y++) {
        // BMP is stored bottom-up
        uint32_t bmpRow = imgHeight - 1 - y;
        const uint8_t* src = bmp.data() + pixelOffset + bmpRow * rowSize;
        uint8_t* dst = rgb.data() + (y * imgWidth * 3);

        for (uint32_t x = 0; x < imgWidth; x++) {
            uint8_t b = src[x * 3 + 0];
            uint8_t g = src[x * 3 + 1];
            uint8_t r = src[x * 3 + 2];

            dst[x * 3 + 0] = r;
            dst[x * 3 + 1] = g;
            dst[x * 3 + 2] = b;
        }
    }

    if (!blitRGBToFramebuffer(rgb.data(), imgWidth, imgHeight)) {
        return false;
    }

    std::cout << "display: showing " << bmpPath
              << " (" << imgWidth << "x" << imgHeight << ")\n";
    return true;
}

bool displayJPEGBuffer(const std::vector<uint8_t>& jpegData) {
    if (jpegData.empty()) {
        std::cerr << "display: empty JPEG buffer\n";
        return false;
    }

    jpeg_decompress_struct cinfo;
    JpegErrorManager jerr;
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpegErrorExit;

    if (setjmp(jerr.setjmpBuffer)) {
        jpeg_destroy_decompress(&cinfo);
        std::cerr << "display: JPEG decode failed (corrupt/incomplete frame)\n";
        return false;
    }

    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, const_cast<unsigned char*>(jpegData.data()), jpegData.size());
    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
        jpeg_destroy_decompress(&cinfo);
        std::cerr << "display: invalid JPEG header\n";
        return false;
    }

    cinfo.out_color_space = JCS_RGB;
    if (!jpeg_start_decompress(&cinfo)) {
        jpeg_destroy_decompress(&cinfo);
        std::cerr << "display: JPEG start decompress failed\n";
        return false;
    }

    const uint32_t width = cinfo.output_width;
    const uint32_t height = cinfo.output_height;
    const uint32_t rowStride = width * cinfo.output_components;
    std::vector<uint8_t> rgb(height * rowStride);

    while (cinfo.output_scanline < cinfo.output_height) {
        JSAMPROW row = rgb.data() + (cinfo.output_scanline * rowStride);
        jpeg_read_scanlines(&cinfo, &row, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    if (!blitRGBToFramebuffer(rgb.data(), width, height)) {
        return false;
    }

    return true;
}

bool displayH264Buffer(const std::vector<uint8_t>& h264Data, bool doBlit) {
    if (h264Data.empty()) {
        return false;
    }

    static AVCodecContext* codecCtx = nullptr;
    static AVFrame* frame = nullptr;
    static AVPacket* pkt = nullptr;
    static SwsContext* swsCtx = nullptr;
    static int swsW = 0;
    static int swsH = 0;
    static AVPixelFormat swsFmt = AV_PIX_FMT_NONE;
    static std::vector<uint8_t> rgb;
    static int rgbW = 0;
    static int rgbH = 0;

    if (!codecCtx) {
        const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!codec) {
            std::cerr << "display: H.264 decoder not found\n";
            return false;
        }
        codecCtx = avcodec_alloc_context3(codec);
        if (codecCtx) {
            codecCtx->flags2 |= AV_CODEC_FLAG2_FAST;
            codecCtx->thread_count = 1;
            // Enable error concealment: decoder fills missing macroblocks
            // using neighboring data instead of producing corrupt output
            codecCtx->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
            codecCtx->flags |= AV_CODEC_FLAG_OUTPUT_CORRUPT;
        }
        frame = av_frame_alloc();
        pkt = av_packet_alloc();
        if (!codecCtx || !frame || !pkt) {
            std::cerr << "display: failed to allocate H.264 decode resources\n";
            return false;
        }
        if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
            std::cerr << "display: failed to open H.264 decoder\n";
            return false;
        }
        g_h264CodecCtx = codecCtx;
    }

    if (static_cast<int>(h264Data.size()) > pkt->size) {
        av_new_packet(pkt, static_cast<int>(h264Data.size()));
    }
    memcpy(pkt->data, h264Data.data(), h264Data.size());
    pkt->size = static_cast<int>(h264Data.size());

    if (avcodec_send_packet(codecCtx, pkt) < 0) {
        std::cerr << "display: failed to send H.264 packet to decoder\n";
        return false;
    }

    bool haveLatest = false;
    int latestW = 0;
    int latestH = 0;
    while (true) {
        int ret = avcodec_receive_frame(codecCtx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            std::cerr << "display: failed to decode H.264 frame\n";
            return false;
        }

        if (!swsCtx || swsW != frame->width || swsH != frame->height || swsFmt != static_cast<AVPixelFormat>(frame->format)) {
            if (swsCtx) {
                sws_freeContext(swsCtx);
                swsCtx = nullptr;
            }
            swsCtx = sws_getContext(
                frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
                frame->width, frame->height, AV_PIX_FMT_RGB24,
                SWS_BILINEAR, nullptr, nullptr, nullptr);
            if (!swsCtx) {
                std::cerr << "display: failed to create swscale context\n";
                return false;
            }
            swsW = frame->width;
            swsH = frame->height;
            swsFmt = static_cast<AVPixelFormat>(frame->format);
        }

        if (rgbW != frame->width || rgbH != frame->height || rgb.empty()) {
            rgbW = frame->width;
            rgbH = frame->height;
            rgb.resize(static_cast<size_t>(rgbW) * static_cast<size_t>(rgbH) * 3);
        }
        uint8_t* dstData[4] = { rgb.data(), nullptr, nullptr, nullptr };
        int dstLinesize[4] = { frame->width * 3, 0, 0, 0 };

        sws_scale(swsCtx,
                  frame->data,
                  frame->linesize,
                  0,
                  frame->height,
                  dstData,
                  dstLinesize);

        latestW = frame->width;
        latestH = frame->height;
        haveLatest = true;
    }

    av_packet_unref(pkt);

    if (haveLatest && doBlit) {
        return blitRGBToFramebuffer(rgb.data(), static_cast<uint32_t>(latestW), static_cast<uint32_t>(latestH));
    }
    return haveLatest;
}

void flushH264Decoder() {
    if (g_h264CodecCtx) {
        avcodec_flush_buffers(g_h264CodecCtx);
    }
}

void clearDisplay() {
    int fb_fd = open(FB_DEV, O_RDWR);
    if (fb_fd < 0) return;
    struct fb_fix_screeninfo finfo;
    ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);
    uint8_t* fb = (uint8_t*)mmap(nullptr, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fb != MAP_FAILED) {
        memset(fb, 0, finfo.smem_len);
        munmap(fb, finfo.smem_len);
    }
    close(fb_fd);
}
