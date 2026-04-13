#ifndef FRAME_BUFFER_POOL_H
#define FRAME_BUFFER_POOL_H

#include <atomic>
#include <cstdint>
#include <cstring>
#include <array>
#include <optional>

// Lock-free single-producer single-consumer ring buffer for video frames
// Designed for real-time video: if buffer is full, oldest frame is dropped

template<size_t FRAME_SIZE, size_t NUM_FRAMES = 3>
class FrameBufferPool {
public:
    struct Frame {
        std::array<uint8_t, FRAME_SIZE> data;
        uint64_t timestamp;     // Monotonic timestamp in microseconds
        uint32_t frameNumber;   // Sequence number for debugging
        bool valid;             // Whether this slot contains valid data
    };

    FrameBufferPool() : writeIdx(0), readIdx(0), droppedFrames(0), totalFrames(0) {
        for (auto& frame : frames) {
            frame.valid = false;
        }
    }

    // Producer: Push a new frame (called from camera callback)
    // Returns true if frame was stored, false if a frame was dropped to make room
    bool push(const uint8_t* frameData, size_t size, uint64_t timestamp) {
        if (size > FRAME_SIZE) {
            return false;  // Frame too large
        }

        uint32_t currentWrite = writeIdx.load(std::memory_order_relaxed);
        uint32_t nextWrite = (currentWrite + 1) % NUM_FRAMES;
        
        // Check if buffer is full (would overtake reader)
        if (nextWrite == readIdx.load(std::memory_order_acquire)) {
            // Drop the oldest frame by advancing read pointer
            readIdx.store((readIdx.load(std::memory_order_relaxed) + 1) % NUM_FRAMES, 
                         std::memory_order_release);
            droppedFrames.fetch_add(1, std::memory_order_relaxed);
        }

        // Write the frame
        Frame& slot = frames[currentWrite];
        std::memcpy(slot.data.data(), frameData, size);
        slot.timestamp = timestamp;
        slot.frameNumber = totalFrames.fetch_add(1, std::memory_order_relaxed);
        slot.valid = true;

        // Publish the write
        writeIdx.store(nextWrite, std::memory_order_release);
        return true;
    }

    // Consumer: Pop the oldest frame (called from encoder thread)
    // Returns nullopt if no frames available
    std::optional<Frame*> pop() {
        uint32_t currentRead = readIdx.load(std::memory_order_relaxed);
        
        // Check if buffer is empty
        if (currentRead == writeIdx.load(std::memory_order_acquire)) {
            return std::nullopt;
        }

        Frame* frame = &frames[currentRead];
        
        // Advance read pointer
        readIdx.store((currentRead + 1) % NUM_FRAMES, std::memory_order_release);
        
        return frame;
    }

    // Check if frames are available without consuming
    bool hasFrames() const {
        return readIdx.load(std::memory_order_acquire) != 
               writeIdx.load(std::memory_order_acquire);
    }

    // Statistics
    uint64_t getDroppedFrames() const { 
        return droppedFrames.load(std::memory_order_relaxed); 
    }
    
    uint64_t getTotalFrames() const { 
        return totalFrames.load(std::memory_order_relaxed); 
    }

    size_t queuedFrames() const {
        uint32_t w = writeIdx.load(std::memory_order_acquire);
        uint32_t r = readIdx.load(std::memory_order_acquire);
        return (w >= r) ? (w - r) : (NUM_FRAMES - r + w);
    }

private:
    std::array<Frame, NUM_FRAMES> frames;
    
    // Atomic indices for lock-free operation
    std::atomic<uint32_t> writeIdx;  // Next slot to write
    std::atomic<uint32_t> readIdx;   // Next slot to read
    
    // Statistics
    std::atomic<uint64_t> droppedFrames;
    std::atomic<uint64_t> totalFrames;
};

#endif // FRAME_BUFFER_POOL_H
