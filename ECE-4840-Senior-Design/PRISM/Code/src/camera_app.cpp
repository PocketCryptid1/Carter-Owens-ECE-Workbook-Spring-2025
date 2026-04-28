#include "camera_app.h"
#include "config.h"
#include <iostream>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <chrono>
#include <thread>

using namespace libcamera;
using namespace std;

// Global flag to signal when the application should exit
extern bool running;

CameraApp::CameraApp() : cameraManager(make_unique<CameraManager>()), pipeline(nullptr), frameCounter(0) {}

CameraApp::~CameraApp() {
    unmapBuffers();
}

void CameraApp::stop() {
    // Stop the camera and pipeline cleanly
    if (camera) {
        camera->stop();
    }
    if (pipeline) {
        pipeline->stop();
    }
    unmapBuffers();
    if (allocator) {
        allocator.reset();
    }
    if (camera) {
        camera->release();
    }
    if (cameraManager) {
        cameraManager->stop();
    }
}

void CameraApp::setEncodedPacketCallback(EncodedPacketCallback callback) {
    encodedPacketCallback = std::move(callback);
    if (pipeline) {
        pipeline->setPacketCallback(encodedPacketCallback);
    }
}

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

    // Initialize the video pipeline (handles encoding in separate thread)
    pipeline = make_unique<VideoPipeline>();
    if (!pipeline->init(CAMERA_WIDTH, CAMERA_HEIGHT, VIDEO_BITRATE, CAMERA_FRAMERATE)) {
        cerr << "Failed to initialize video pipeline\n";
        return -1;
    }

    if (encodedPacketCallback) {
        pipeline->setPacketCallback(encodedPacketCallback);
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

    // Pre-map all buffers to avoid mmap/munmap overhead per frame
    if (!mapBuffers()) {
        cerr << "Failed to pre-map buffers\n";
        return -1;
    }

    // Connect the requestCompleted signal to our handler method
    // This will be called whenever a frame capture completes
    camera->requestCompleted.connect(this, &CameraApp::onRequestComplete);

    // Start the video pipeline (encoder thread)
    pipeline->start();

    // Start the camera capturing frames
    if (camera->start()) {
        cerr << "Camera failed to start\n";
        return -1;
    }

    // Queue all requests to the camera to begin capturing frames
    for (auto &request : requests) {
        camera->queueRequest(request.get());
    }

    cout << "Camera running at " << CAMERA_WIDTH << "x" << CAMERA_HEIGHT 
         << " @ " << CAMERA_FRAMERATE << " fps... Press Ctrl+C to exit\n";

    // Main loop: Keep application running until interrupted
    // Print stats periodically
    auto lastStats = chrono::steady_clock::now();
    lastFrameTime = std::chrono::steady_clock::now();
    while (running) {
        usleep(100000);  // 100ms sleep
        
        auto now = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::seconds>(now - lastStats).count() >= 2) {
            cout << "[Stats] Total: " << pipeline->getTotalFrames() 
                 << " | Dropped: " << pipeline->getDroppedFrames()
                 << " | Queued: " << pipeline->getQueuedFrames() << "\n";
            lastStats = now;
        }
    }

    // Graceful shutdown sequence
    cout << "Shutting down...\n";
    stop();
    return 0;
}

bool CameraApp::mapBuffers() {
    for (StreamConfiguration &cfg : *config) {
        for (const unique_ptr<FrameBuffer> &buffer : allocator->buffers(cfg.stream())) {
            const vector<FrameBuffer::Plane> &planes = buffer->planes();
            
            if (planes.empty()) continue;

            // Calculate total buffer size
            uint32_t totalSize = 0;
            for (const auto &plane : planes) {
                uint32_t planeEnd = plane.offset + plane.length;
                if (planeEnd > totalSize) {
                    totalSize = planeEnd;
                }
            }

            // Map the entire buffer once
            int fd = planes[0].fd.get();
            void* mapped = mmap(nullptr, totalSize, PROT_READ, MAP_SHARED, fd, 0);
            
            if (mapped == MAP_FAILED) {
                cerr << "Failed to pre-map buffer (errno: " << errno << ")\n";
                return false;
            }

            mappedBuffers[buffer.get()] = {mapped, totalSize};
        }
    }
    
    cout << "Pre-mapped " << mappedBuffers.size() << " buffers\n";
    return true;
}

void CameraApp::unmapBuffers() {
    for (auto& [buffer, mapped] : mappedBuffers) {
        if (mapped.data && mapped.data != MAP_FAILED) {
            munmap(mapped.data, mapped.size);
        }
    }
    mappedBuffers.clear();
}

void CameraApp::onRequestComplete(Request *request) {
    // Skip processing if the request was cancelled
    if (request->status() == Request::RequestCancelled)
        return;

    // Get the buffers associated with this completed request
    const Request::BufferMap &buffers = request->buffers();
    for (auto bufferPair : buffers) {
        FrameBuffer *buffer = bufferPair.second;
        if (!buffer) continue;

        // Look up pre-mapped buffer
        auto it = mappedBuffers.find(buffer);
        if (it == mappedBuffers.end()) {
            // This shouldn't happen if mapBuffers() succeeded
            continue;
        }

        const vector<FrameBuffer::Plane> &planes = buffer->planes();
        if (planes.empty()) continue;

        // Fast path: copy YUV data from pre-mapped buffer
        // For contiguous YUV420, data is already laid out correctly
        uint8_t* basePtr = static_cast<uint8_t*>(it->second.data);
        
        // Calculate expected frame size
        size_t frameSize = CAMERA_WIDTH * CAMERA_HEIGHT * 3 / 2;
        
        // Submit to pipeline (this just copies to ring buffer - very fast)
        pipeline->submitFrame(basePtr, frameSize);
        
        // Increment frame counter and optionally stop when a fixed frame limit is set.
        frameCounter++;
        if (FRAMES_TO_RECORD > 0 && frameCounter >= FRAMES_TO_RECORD) {
            cout << "Recorded " << RECORDING_DURATION_SECONDS << " seconds of video (" 
                 << frameCounter << " frames). Shutting down...\n";
            running = false;  // Trigger graceful shutdown
        }

        // Real-time pacing: enforce frame interval
        constexpr int frameIntervalMs = 1000 / CAMERA_FRAMERATE;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime).count();
        if (elapsed < frameIntervalMs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(frameIntervalMs - elapsed));
        }
        lastFrameTime = std::chrono::steady_clock::now();
    }

    // Prepare the request for reuse with the same buffers
    // This keeps the same memory allocated to avoid reallocation overhead
    request->reuse(Request::ReuseBuffers);
    // Queue the request again to capture the next frame
    camera->queueRequest(request);
}
