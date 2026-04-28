#ifndef CAMERA_APP_H
#define CAMERA_APP_H

#include <libcamera/libcamera.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include "video_pipeline.h"

using namespace libcamera;

// Pre-mapped buffer info to avoid mmap/munmap per frame
struct MappedBuffer {
    void* data;
    size_t size;
};

// CameraApp encapsulates all camera functionality and management
// Handles camera initialization, configuration, buffer allocation, and frame capture
class CameraApp {
public:
    // Constructor: Initialize the CameraManager which manages all camera devices
    CameraApp();
    
    // Destructor: Clean up mapped buffers
    ~CameraApp();

    // Main execution function: Sets up and runs the camera application
    // Returns 0 on success, -1 on any error
    int run();

    // Set callback for encoded H.264 packet bytes.
    void setEncodedPacketCallback(EncodedPacketCallback callback);

    // Stop the camera and pipeline cleanly
    void stop();

private:
    // Callback handler: Called asynchronously when a frame capture completes
    void onRequestComplete(Request *request);
    
    // Pre-map all buffers at startup to avoid per-frame mmap overhead
    bool mapBuffers();
    
    // Unmap all buffers at shutdown
    void unmapBuffers();

    // Member variables - the state of the camera application
    std::unique_ptr<CameraManager> cameraManager;  // Manages all camera devices
    std::shared_ptr<Camera> camera;                // The camera device we're using
    std::unique_ptr<CameraConfiguration> config;   // Camera configuration (resolution, format, etc)
    std::unique_ptr<FrameBufferAllocator> allocator;// Allocates memory for frame buffers
    std::unique_ptr<VideoPipeline> pipeline;       // Async video encoding pipeline
    std::vector<std::unique_ptr<Request>> requests; // Queue of requests with buffers for continuous capture
    
    // Pre-mapped buffer cache: FrameBuffer* -> MappedBuffer
    std::unordered_map<FrameBuffer*, MappedBuffer> mappedBuffers;
    
    int frameCounter;                              // Count frames to trigger shutdown

    // For real-time pacing
    std::chrono::steady_clock::time_point lastFrameTime;

    // Optional encoded packet callback wired into VideoPipeline when available.
    EncodedPacketCallback encodedPacketCallback;
};

#endif // CAMERA_APP_H
