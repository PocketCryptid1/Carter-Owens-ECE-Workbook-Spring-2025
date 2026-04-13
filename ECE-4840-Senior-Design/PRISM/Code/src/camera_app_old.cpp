#include "camera_app.h"
#include "config.h"
#include "h264_encoder.h"
#include <iostream>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

using namespace libcamera;
using namespace std;

// Global flag to signal when the application should exit
extern bool running;

CameraApp::CameraApp() : cameraManager(make_unique<CameraManager>()), encoder(nullptr), frameCounter(0) {}

int CameraApp::run() {
    // Start the CameraManager to enumerate all available camera devices
    if (cameraManager->start()) {
        cerr << "Failed to start CameraManager\n";
        return -1;
    }

    // Check if at least one camera is available
    if (cameraManager->cameras().empty()) {
        cerr << "No cameras found\n";
        return -1;
    }

    // Get the first camera device (usually the only one on Raspberry Pi)
    camera = cameraManager->cameras()[0];
    // Acquire exclusive access to the camera
    camera->acquire();

    // Configure camera
    // Generate a configuration for video recording stream role
    config = camera->generateConfiguration({ StreamRole::VideoRecording });
    // Get the first (and only) stream configuration
    StreamConfiguration &streamConfig = config->at(0);
    // Set resolution to configured dimensions
    streamConfig.size.width = CAMERA_WIDTH;
    streamConfig.size.height = CAMERA_HEIGHT;
    // Use YUV420 pixel format (common for video, more efficient than RGB)
    streamConfig.pixelFormat = formats::YUV420;

    // Validate the configuration to ensure camera can support it
    if (config->validate() == CameraConfiguration::Invalid) {
        cerr << "Invalid configuration\n";
        return -1;
    }

    // Apply the configuration to the camera
    if (camera->configure(config.get()) < 0) {
        cerr << "Camera configuration failed\n";
        return -1;
    }

    // Initialize H.264 hardware encoder
    encoder = make_unique<H264Encoder>();
    if (!encoder->init(CAMERA_WIDTH, CAMERA_HEIGHT, VIDEO_BITRATE, CAMERA_FRAMERATE)) {
        cerr << "Failed to initialize H.264 encoder\n";
        return -1;
    }

    // Allocate memory buffers for storing captured frames
    allocator = make_unique<FrameBufferAllocator>(camera);
    // For each stream in the configuration, allocate buffers
    for (StreamConfiguration &cfg : *config) {
        if (allocator->allocate(cfg.stream()) < 0) {
            cerr << "Buffer allocation failed\n";
            return -1;
        }

        // Create a request for each allocated buffer
        // Requests are how libcamera submits work to the camera
        for (const unique_ptr<FrameBuffer> &buffer : allocator->buffers(cfg.stream())) {
            // Create a new capture request
            unique_ptr<Request> request = camera->createRequest();
            if (!request) {
                cerr << "Failed to create request\n";
                return -1;
            }

            // Associate the buffer with this request
            if (request->addBuffer(cfg.stream(), buffer.get()) < 0) {
                cerr << "Failed to add buffer\n";
                return -1;
            }

            // Store the request in our list for queueing
            requests.push_back(move(request));
        }
    }

    // Connect the requestCompleted signal to our handler method
    // This will be called whenever a frame capture completes
    camera->requestCompleted.connect(this, &CameraApp::onRequestComplete);

    // Start the camera capturing frames
    if (camera->start()) {
        cerr << "Camera failed to start\n";
        return -1;
    }

    // Queue all requests to the camera to begin capturing frames
    for (auto &request : requests) {
        camera->queueRequest(request.get());
    }

    cout << "Camera running... Press Ctrl+C to exit\n";

    // Main loop: Keep application running until interrupted
    // The actual frame processing happens asynchronously in onRequestComplete()
    while (running) {
        sleep(1);
    }

    // Graceful shutdown sequence
    cout << "Shutting down...\n";
    camera->stop();
    encoder->finalize();  // Finalize encoding and close file
    allocator.reset();
    camera->release();
    cameraManager->stop();

    return 0;
}

void CameraApp::onRequestComplete(Request *request) {
    // Skip processing if the request was cancelled
    if (request->status() == Request::RequestCancelled)
        return;

    cout << "Frame received - encoding to H.265\n";

    // Get the buffers associated with this completed request
    const Request::BufferMap &buffers = request->buffers();
    for (auto bufferPair : buffers) {
        FrameBuffer *buffer = bufferPair.second;
        if (!buffer)
            continue;

        // Get the planes of the frame buffer (YUV420 has Y, U, V planes)
        const std::vector<FrameBuffer::Plane> &planes = buffer->planes();
        cout << "Number of planes: " << planes.size() << "\n";
        
        if (planes.empty())
            continue;

        std::vector<uint8_t> frameData;
        bool frameValid = true;

        // When multiple planes share the same fd, we need to mmap the entire buffer
        // and access planes through pointer arithmetic
        if (planes.size() > 1 && planes[0].fd.get() == planes[planes.size()-1].fd.get()) {
            cout << "Contiguous buffer mode: all planes in single fd\n";
            
            // Find the total buffer size (from offset 0 to the end of the last plane)
            uint32_t totalSize = 0;
            for (const auto &plane : planes) {
                uint32_t planeEnd = plane.offset + plane.length;
                if (planeEnd > totalSize) {
                    totalSize = planeEnd;
                }
            }
            cout << "Total buffer size: " << totalSize << " bytes\n";

            // Mmap the entire buffer from offset 0
            int fd = planes[0].fd.get();
            auto bufferBase = mmap(nullptr, totalSize, PROT_READ, MAP_SHARED, fd, 0);
            
            if (bufferBase == MAP_FAILED) {
                cerr << "Failed to map buffer (errno: " << errno << ")\n";
                frameValid = false;
            } else {
                // Copy all planes using pointer arithmetic
                for (size_t planeIdx = 0; planeIdx < planes.size(); ++planeIdx) {
                    const FrameBuffer::Plane &plane = planes[planeIdx];
                    uint8_t *planePtr = (uint8_t*)bufferBase + plane.offset;
                    cout << "Plane " << planeIdx << ": offset=" << plane.offset 
                         << ", length=" << plane.length << "\n";
                    frameData.insert(frameData.end(), planePtr, planePtr + plane.length);
                }
                munmap(bufferBase, totalSize);
            }
        }
        // Fallback for truly separate planes (different fds)
        else {
            cout << "Separate planes mode\n";
            
            for (size_t planeIdx = 0; planeIdx < planes.size(); ++planeIdx) {
                const FrameBuffer::Plane &plane = planes[planeIdx];
                cout << "Plane " << planeIdx << ": fd=" << plane.fd.get() 
                     << ", offset=" << plane.offset << ", length=" << plane.length << "\n";
                
                if (plane.fd.get() < 0) {
                    cerr << "Invalid file descriptor for plane " << planeIdx << "\n";
                    frameValid = false;
                    break;
                }

                auto planeData = mmap(nullptr, plane.length, PROT_READ, MAP_SHARED,
                                     plane.fd.get(), plane.offset);
                if (planeData == MAP_FAILED) {
                    cerr << "Failed to map plane " << planeIdx << " (errno: " << errno << ")\n";
                    frameValid = false;
                    break;
                }

                frameData.insert(frameData.end(), 
                                (uint8_t*)planeData, 
                                (uint8_t*)planeData + plane.length);

                munmap(planeData, plane.length);
            }
        }

        // Send the complete frame to the H.265 encoder if all planes mapped successfully
        if (frameValid && !frameData.empty() && encoder) {
            cout << "Sending " << frameData.size() << " bytes to encoder\n";
            encoder->encodeFrame(frameData.data(), frameData.size());
            
            // Increment frame counter and check if we've reached the target duration
            frameCounter++;
            if (frameCounter >= FRAMES_TO_RECORD) {
                cout << "Recorded " << RECORDING_DURATION_SECONDS << " seconds of video (" 
                     << frameCounter << " frames). Shutting down...\n";
                running = false;  // Trigger graceful shutdown
            }
        } else if (!frameValid) {
            cerr << "Skipping frame due to buffer mapping errors\n";
        }
    }

    // Prepare the request for reuse with the same buffers
    // This keeps the same memory allocated to avoid reallocation overhead
    request->reuse(Request::ReuseBuffers);
    // Queue the request again to capture the next frame
    camera->queueRequest(request);
}
