#ifndef CAMERA_APP_H
#define CAMERA_APP_H

#include <libcamera/libcamera.h>
#include <memory>
#include <vector>
#include "h265_encoder.h"

using namespace libcamera;

// CameraApp encapsulates all camera functionality and management
// Handles camera initialization, configuration, buffer allocation, and frame capture
class CameraApp {
public:
    // Constructor: Initialize the CameraManager which manages all camera devices
    CameraApp();

    // Main execution function: Sets up and runs the camera application
    // Returns 0 on success, -1 on any error
    int run();

private:
    // Callback handler: Called asynchronously when a frame capture completes
    void onRequestComplete(Request *request);

    // Member variables - the state of the camera application
    std::unique_ptr<CameraManager> cameraManager;  // Manages all camera devices
    std::shared_ptr<Camera> camera;                // The camera device we're using
    std::unique_ptr<CameraConfiguration> config;   // Camera configuration (resolution, format, etc)
    std::unique_ptr<FrameBufferAllocator> allocator;// Allocates memory for frame buffers
    std::unique_ptr<H265Encoder> encoder;          // H.265 hardware video encoder
    std::vector<std::unique_ptr<Request>> requests; // Queue of requests with buffers for continuous capture
    int frameCounter;                              // Count frames to trigger 5-second shutdown
};

#endif // CAMERA_APP_H
