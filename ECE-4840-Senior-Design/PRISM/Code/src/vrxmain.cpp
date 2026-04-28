#include "protocol.h"
#include "config.h"
#include "display.h"
#include <iostream>
#include <pcap.h>
#include <cstring>
#include <csignal>
#include <vector>
#include <chrono>
extern "C" {
#include <libavutil/log.h>
}

using namespace std;

bool running = true;
pcap_t* global_handle = nullptr;

static const uint8_t VTX_SOURCE_MAC[6] = {0x54, 0xc9, 0xff, 0x02, 0xe8, 0xbc};
static const uint8_t VRX_ACK_SOURCE_MAC_BYTES[6] = VRX_ACK_SOURCE_MAC;

static const uint8_t ACK_RADIOTAP_HDR[] = {
    0x00, 0x00,
    0x0c, 0x00,
    0x04, 0x80, 0x00, 0x00,
    0x00, 0x00,
    TX_RATE_500KBPS_UNITS,
    0x00,
};

bool sendAckPacket(pcap_t* handle, const uint8_t* dstMac, const PacketHeader& rxPkt) {
    if (!handle || !dstMac) return false;

    uint8_t dot11Hdr[24] = {
        0x40, 0x00, 0x00, 0x00,
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00,
    };
    memcpy(dot11Hdr + 4, dstMac, 6);
    memcpy(dot11Hdr + 10, VRX_ACK_SOURCE_MAC_BYTES, 6);

    uint8_t dummy = 0;
    Packet ackPkt(rxPkt.framecounter, rxPkt.chunknumber, rxPkt.totalchunks, &dummy, 0, FLAG_IS_ACK);
    uint8_t payloadBuf[sizeof(PacketHeader) + PAYLOAD_MAX_SIZE];
    ackPkt.serialize(payloadBuf, sizeof(payloadBuf));

    uint8_t frame[sizeof(ACK_RADIOTAP_HDR) + sizeof(dot11Hdr) + sizeof(PacketHeader)];
    size_t outLen = sizeof(ACK_RADIOTAP_HDR) + sizeof(dot11Hdr) + ackPkt.packetSize;
    memcpy(frame, ACK_RADIOTAP_HDR, sizeof(ACK_RADIOTAP_HDR));
    memcpy(frame + sizeof(ACK_RADIOTAP_HDR), dot11Hdr, sizeof(dot11Hdr));
    memcpy(frame + sizeof(ACK_RADIOTAP_HDR) + sizeof(dot11Hdr), payloadBuf, ackPkt.packetSize);

    return pcap_inject(handle, frame, outLen) != -1;
}

struct RxState {
    pcap_t* txHandle = nullptr;
    uint32_t currentFrame = 0;
    uint16_t totalChunks = 0;
    uint8_t currentFlags = 0;
    std::vector<std::vector<uint8_t>> chunks;
    std::vector<bool> chunkReceived;
    uint16_t receivedCount = 0;
    std::chrono::steady_clock::time_point lastPacketTime;
    std::chrono::steady_clock::time_point lastStatsPrint;
    std::chrono::steady_clock::time_point lastDisplayTime;
    bool showingIdle = true;

    // Telemetry counters for debugging RF vs parsing vs display.
    uint64_t packetsSeen = 0;
    uint64_t packetsFromVtx = 0;
    uint64_t packetsMagicValid = 0;
    uint64_t headerSizeMismatch = 0;
    uint64_t payloadSizeInvalid = 0;
    uint64_t chunksAccepted = 0;
    uint64_t duplicateChunks = 0;
    uint64_t framesCompleted = 0;
    uint64_t jpegDisplayOk = 0;
    uint64_t jpegDisplayFail = 0;
    uint64_t idleFallbacks = 0;
    uint64_t assemblyResyncs = 0;
    uint64_t framesDroppedByRateLimit = 0;
    uint64_t framesAbandoned = 0;    // frames discarded incomplete due to new frame arriving
    uint64_t chunksExpected = 0;     // total chunks expected across all started frames
    uint64_t chunksLost = 0;         // expected minus received (RF loss estimate)
    uint64_t ackSent = 0;
    uint64_t ackFailed = 0;
};

static void startNewAssembly(RxState* state, uint32_t framecounter, uint16_t totalchunks, uint8_t flags) {
    state->currentFrame = framecounter;
    state->totalChunks = totalchunks;
    state->currentFlags = flags;
    state->chunks.assign(totalchunks, {});
    state->chunkReceived.assign(totalchunks, false);
    state->receivedCount = 0;
}

void signalHandler(int) {
    cout << "\nShutting down VRX...\n";
    running = false;
    if (global_handle) pcap_breakloop(global_handle);
}

// pcap callback — called for every captured packet
void packetHandler(u_char* user, const struct pcap_pkthdr* header, const u_char* data) {
    RxState* state = reinterpret_cast<RxState*>(user);
    if (!state) return;
    state->packetsSeen++;

    // Minimum size: radiotap + 802.11 header + PacketHeader
    const size_t MIN_SIZE = 12 + 24 + sizeof(PacketHeader);
    if (header->caplen < MIN_SIZE) return;

    // Skip radiotap header (length is at bytes 2-3, little-endian)
    uint16_t rt_len = data[2] | (data[3] << 8);
    if (header->caplen < rt_len + 24 + sizeof(PacketHeader)) return;

    // Skip 802.11 header (24 bytes) to get to payload
    const u_char* payload = data + rt_len + 24;
    size_t payload_len = header->caplen - rt_len - 24;

    // Address2 (source) in 802.11 header starts at byte 10 from 802.11 header start.
    const u_char* dot11 = data + rt_len;
    if (memcmp(dot11 + 10, VTX_SOURCE_MAC, 6) != 0) return;
    state->packetsFromVtx++;

    if (payload_len < sizeof(PacketHeader)) return;

    // Interpret payload as a PacketHeader and validate magic number
    PacketHeader pkt;
    memcpy(&pkt, payload, sizeof(PacketHeader));

    if (!pkt.isValid()) return;
    state->packetsMagicValid++;
    if (pkt.totalchunks == 0 || pkt.chunknumber >= pkt.totalchunks) return;
    if (pkt.headersize != sizeof(PacketHeader)) {
        state->headerSizeMismatch++;
        return;
    }
    if (pkt.payloadsize > PAYLOAD_MAX_SIZE) {
        state->payloadSizeInvalid++;
        return;
    }

    const uint8_t* chunkData = payload + sizeof(PacketHeader);
    const size_t availableLen = payload_len - sizeof(PacketHeader);
    const size_t chunkLen = std::min<size_t>(pkt.payloadsize, availableLen);
    if (chunkLen == 0) return;

    if (pkt.flags & FLAG_ACK_REQUESTED) {
        if (sendAckPacket(state->txHandle, dot11 + 10, pkt)) state->ackSent++;
        else state->ackFailed++;
    }

    state->lastPacketTime = std::chrono::steady_clock::now();

    if (state->showingIdle) {
        state->showingIdle = false;
    }

    // Reject obviously invalid chunk counts to avoid pathological allocations.
    if (pkt.totalchunks == 0 || pkt.totalchunks > 4096) {
        return;
    }

    // Start a new frame assembly when frame counter changes.
    if (state->chunks.empty() || pkt.framecounter != state->currentFrame) {
        // Drop incomplete frame immediately to minimize latency and avoid extra decode load.
        if (!state->chunks.empty() && state->receivedCount < state->totalChunks) {
            state->framesAbandoned++;
            state->chunksLost += state->totalChunks - state->receivedCount;
        }
        state->chunksExpected += pkt.totalchunks;
        startNewAssembly(state, pkt.framecounter, pkt.totalchunks, pkt.flags);
    }

    // If metadata changed unexpectedly while framecounter stayed same, resync instead
    // of getting stuck forever on a corrupted first header.
    if (pkt.totalchunks != state->totalChunks) {
        state->assemblyResyncs++;
        startNewAssembly(state, pkt.framecounter, pkt.totalchunks, pkt.flags);
    }

    if (!state->chunkReceived[pkt.chunknumber]) {
        state->chunks[pkt.chunknumber].assign(chunkData, chunkData + chunkLen);
        state->chunkReceived[pkt.chunknumber] = true;
        state->receivedCount++;
        state->chunksAccepted++;
    } else {
        state->duplicateChunks++;
    }

    if (state->receivedCount == state->totalChunks) {
        size_t totalBytes = 0;
        for (const auto& c : state->chunks) {
            totalBytes += c.size();
        }

        std::vector<uint8_t> imageData;
        imageData.reserve(totalBytes);
        for (const auto& c : state->chunks) {
            imageData.insert(imageData.end(), c.begin(), c.end());
        }

        bool displayed = false;
        auto now = std::chrono::steady_clock::now();
        auto minFrameGap = std::chrono::milliseconds(1000 / VRX_MAX_DISPLAY_FPS);
        bool shouldDisplay = (state->lastDisplayTime.time_since_epoch().count() == 0) ||
                             (now - state->lastDisplayTime >= minFrameGap);

        if (state->currentFlags & FLAG_IS_H264) {
            if (shouldDisplay) {
                // Decode and blit
                displayed = displayH264Buffer(imageData, true);
                if (displayed) {
                    state->lastDisplayTime = now;
                    state->jpegDisplayOk++;
                } else {
                    // Real decode/blit failure — flush decoder so next keyframe resyncs cleanly
                    flushH264Decoder();
                    state->jpegDisplayFail++;
                }
            } else {
                // Decode only — keeps reference chain intact, don't count as fail
                displayH264Buffer(imageData, false);
                state->framesDroppedByRateLimit++;
            }
        } else {
            // JPEG: each frame is independent, safe to skip entirely
            if (shouldDisplay) {
                displayed = displayJPEGBuffer(imageData);
                if (displayed) {
                    state->lastDisplayTime = now;
                    state->jpegDisplayOk++;
                } else {
                    state->jpegDisplayFail++;
                }
            } else {
                state->framesDroppedByRateLimit++;
            }
        }

        state->framesCompleted++;

        state->chunks.clear();
        state->chunkReceived.clear();
        state->receivedCount = 0;
        state->totalChunks = 0;
    }
}

int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Suppress FFmpeg concealment spam — errors are expected/handled, not actionable.
    av_log_set_level(AV_LOG_QUIET);

    const char* iface = NETWORK_INTERFACE;
    char errbuf[PCAP_ERRBUF_SIZE];

    cout << "VRX listening on " << iface << "...\n";

    // Show idle screen on HDMI while waiting for signal
    displayBMP("/home/prism/PRISM/media/idlescreen.bmp");

    RxState state;
    state.lastPacketTime = std::chrono::steady_clock::now();
    state.lastStatsPrint = state.lastPacketTime;
    state.lastDisplayTime = std::chrono::steady_clock::time_point{};
    state.showingIdle = true;

    pcap_t* handle = pcap_create(iface, errbuf);
    if (!handle) {
        cerr << "Failed to create pcap handle on " << iface << ": " << errbuf << "\n";
        return 1;
    }
    pcap_set_snaplen(handle, 2048);
    pcap_set_promisc(handle, 1);
    pcap_set_buffer_size(handle, 128 * 1024);
    pcap_set_timeout(handle, 1);
    pcap_set_immediate_mode(handle, 1);
    int act = pcap_activate(handle);
    if (act != 0) {
        cerr << "Failed to activate interface " << iface << ": " << pcap_statustostr(act) << "\n";
        pcap_close(handle);
        return 1;
    }
    state.txHandle = handle;
    global_handle = handle;

    // Capture loop — runs until Ctrl+C
    while (running) {
        pcap_dispatch(handle, 32, packetHandler, reinterpret_cast<u_char*>(&state));

        auto now = std::chrono::steady_clock::now();
        auto silenceMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - state.lastPacketTime).count();
        if (silenceMs > 1500 && !state.showingIdle) {
            flushH264Decoder();
            displayBMP("/home/prism/PRISM/media/idlescreen.bmp");
            state.showingIdle = true;
            state.idleFallbacks++;
            state.chunks.clear();
            state.chunkReceived.clear();
            state.receivedCount = 0;
            state.totalChunks = 0;
        }

        auto statsMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - state.lastStatsPrint).count();
        if (statsMs >= 1000) {
            // Compute chunk loss % for RF diagnostics
            float lossPercent = (state.chunksExpected > 0)
                ? (100.0f * state.chunksLost / state.chunksExpected) : 0.0f;
            cout << "[VRX stats]"
                 << " frames=" << state.framesCompleted
                 << " abandoned=" << state.framesAbandoned
                 << " loss%=" << static_cast<int>(lossPercent)
                 << " dup=" << state.duplicateChunks
                 << " resyncs=" << state.assemblyResyncs
                 << " ok=" << state.jpegDisplayOk
                 << " fail=" << state.jpegDisplayFail
                 << " skip=" << state.framesDroppedByRateLimit
                 << " ack_tx=" << state.ackSent
                 << " ack_fail=" << state.ackFailed
                 << " silence_ms=" << silenceMs
                 << "\n";
            state.lastStatsPrint = now;
        }
    }

    pcap_close(handle);
    global_handle = nullptr;
    clearDisplay();
    return 0;
}
