#ifndef VIDEO_PIPELINE_H
#define VIDEO_PIPELINE_H

#include "frame_buffer_pool.h"
#include "h264_encoder.h"
#include "config.h"
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>
#include <iostream>

// Calculate frame size for YUV420: width * height * 1.5
constexpr size_t FRAME_SIZE = CAMERA_WIDTH * CAMERA_HEIGHT * 3 / 2;

// Callback type for encoded packets (will be used by transmitter)
using EncodedPacketCallback = std::function<void(const uint8_t* data, size_t size, uint64_t timestamp)>;

// VideoPipeline manages the asynchronous encoding thread
// Decouples camera capture from encoding to prevent blocking
class VideoPipeline {
public:
    VideoPipeline() : encoder(nullptr), running(false) {}
    
    ~VideoPipeline() {
        stop();
    }

    // Initialize the pipeline with encoder settings
    bool init(uint32_t width, uint32_t height, uint32_t bitrate, uint32_t framerate) {
        encoder = std::make_unique<H264Encoder>();
        if (!encoder->init(width, height, bitrate, framerate)) {
            return false;
        }

        if (packetCallback) {
            encoder->setEncodedPacketCallback(packetCallback);
        }
        
        this->width = width;
        this->height = height;
        
        return true;
    }

    // Start the encoder thread
    void start() {
        if (running.load()) return;
        
        running.store(true);
        encoderThread = std::thread(&VideoPipeline::encoderLoop, this);
    }

    // Stop the encoder thread
    void stop() {
        running.store(false);
        if (encoderThread.joinable()) {
            encoderThread.join();
        }
        if (encoder) {
            encoder->finalize();
        }
    }

    // Submit a frame for encoding (called from camera callback - must be fast!)
    void submitFrame(const uint8_t* data, size_t size) {
        auto now = std::chrono::steady_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();
        
        frameBuffer.push(data, size, timestamp);
    }

    // Set callback for encoded packets (for future transmission)
    void setPacketCallback(EncodedPacketCallback callback) {
        packetCallback = std::move(callback);
        if (encoder) {
            encoder->setEncodedPacketCallback(packetCallback);
        }
    }

    // Get statistics
    uint64_t getDroppedFrames() const { return frameBuffer.getDroppedFrames(); }
    uint64_t getTotalFrames() const { return frameBuffer.getTotalFrames(); }
    size_t getQueuedFrames() const { return frameBuffer.queuedFrames(); }

private:
    void encoderLoop() {
        std::cout << "Encoder thread started\n";
        
        while (running.load()) {
            auto frameOpt = frameBuffer.pop();
            
            if (frameOpt.has_value()) {
                auto* frame = frameOpt.value();
                
                // Encode the frame
                encoder->encodeFrame(frame->data.data(), FRAME_SIZE);
                
            } else {
                // No frame available, sleep briefly to avoid busy-waiting
                std::this_thread::sleep_for(std::chrono::microseconds(500));
            }
        }
        
        std::cout << "Encoder thread stopped. Stats: "
                  << "Total frames: " << frameBuffer.getTotalFrames()
                  << ", Dropped: " << frameBuffer.getDroppedFrames() << "\n";
    }

    // Frame buffer pool (lock-free ring buffer)
    FrameBufferPool<FRAME_SIZE, 3> frameBuffer;
    
    // Encoder
    std::unique_ptr<H264Encoder> encoder;
    
    // Encoder thread
    std::thread encoderThread;
    std::atomic<bool> running;
    
    // Dimensions (for validation)
    uint32_t width, height;
    
    // Callback for encoded packets
    EncodedPacketCallback packetCallback;
};

#endif // VIDEO_PIPELINE_H
