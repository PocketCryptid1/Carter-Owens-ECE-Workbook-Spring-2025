#include "protocol.h"
#include "config.h"
#include "camera_app.h"
#include <iostream>
#include <csignal>
#include <pcap.h>
#include <cstring>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <atomic>
#include <chrono>

using namespace std;

bool running = true;
pcap_t* global_handle = nullptr;
atomic<uint32_t> encodedFrameCounter{1};

void signalHandler(int) {
    cout << "\nShutting down VTX...\n";
    running = false;
    if (global_handle) pcap_breakloop(global_handle);
}

// Radiotap header — minimal, 12 bytes, configurable legacy rate
static const uint8_t RADIOTAP_HDR[] = {
    0x00, 0x00,             // version + pad
    0x0c, 0x00,             // header length: 12 bytes
    0x04, 0x80, 0x00, 0x00, // present flags: TX flags + rate
    0x00, 0x00,             // TX flags (none) + pad
    TX_RATE_500KBPS_UNITS,  // rate in 500 kbps units (default 6 Mbps)
    0x00,                   // pad
};

// 802.11 management frame header (probe request, broadcast) — 24 bytes
static const uint8_t DOT11_HDR[] = {
    0x40, 0x00, 0x00, 0x00,             // Frame Control (probe req / mgmt), Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination: broadcast
    0x54, 0xc9, 0xff, 0x02, 0xe8, 0xbc, // Source: VTX MAC
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // BSSID: broadcast
    0x00, 0x00,                         // Sequence control
};

static const size_t RT_LEN   = sizeof(RADIOTAP_HDR);
static const size_t DOT11_LEN = sizeof(DOT11_HDR);

struct AckWaitState {
    uint32_t framecounter = 0;
    uint16_t chunknumber = 0;
    bool acked = false;
};

void ackPacketHandler(u_char* user, const struct pcap_pkthdr* header, const u_char* data) {
    AckWaitState* state = reinterpret_cast<AckWaitState*>(user);
    if (!state || !header || !data) return;

    const size_t minSize = 12 + 24 + sizeof(PacketHeader);
    if (header->caplen < minSize) return;

    uint16_t rtLen = data[2] | (data[3] << 8);
    if (header->caplen < rtLen + 24 + sizeof(PacketHeader)) return;

    const u_char* payload = data + rtLen + 24;
    PacketHeader pkt;
    memcpy(&pkt, payload, sizeof(PacketHeader));

    if (!pkt.isValid()) return;
    if (!(pkt.flags & FLAG_IS_ACK)) return;
    if (pkt.framecounter != state->framecounter) return;
    if (pkt.chunknumber != state->chunknumber) return;

    state->acked = true;
}

bool waitForAck(pcap_t* handle, uint32_t framecounter, uint16_t chunknumber, int timeoutMs) {
    if (!handle) return false;

    AckWaitState waitState;
    waitState.framecounter = framecounter;
    waitState.chunknumber = chunknumber;

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (running && std::chrono::steady_clock::now() < deadline) {
        int rc = pcap_dispatch(handle, 16, ackPacketHandler, reinterpret_cast<u_char*>(&waitState));
        if (waitState.acked) return true;
        if (rc < 0) return false;
        if (rc == 0) usleep(200);
    }

    return waitState.acked;
}

// Inject a single serialized PRISM packet over the air
bool injectPacket(pcap_t* handle, const uint8_t* pkt_data, size_t pkt_len) {
    uint8_t frame[RT_LEN + DOT11_LEN + sizeof(PacketHeader) + PAYLOAD_MAX_SIZE];
    size_t frame_len = RT_LEN + DOT11_LEN + pkt_len;

    memcpy(frame,                    RADIOTAP_HDR, RT_LEN);
    memcpy(frame + RT_LEN,           DOT11_HDR,    DOT11_LEN);
    memcpy(frame + RT_LEN + DOT11_LEN, pkt_data,  pkt_len);

    if (pcap_inject(handle, frame, frame_len) == -1) {
        cerr << "Inject error: " << pcap_geterr(handle) << "\n";
        return false;
    }
    return true;
}

bool readBinaryFile(const string& path, vector<uint8_t>& out) {
    ifstream f(path, ios::binary);
    if (!f) {
        cerr << "Failed to open image file: " << path << "\n";
        return false;
    }
    out.assign(istreambuf_iterator<char>(f), istreambuf_iterator<char>());
    if (out.empty()) {
        cerr << "Image file is empty: " << path << "\n";
        return false;
    }
    return true;
}

// Chunk JPEG bytes into PRISM packets and transmit as one frame.
// For link bring-up, we can retransmit chunks to make frame completion more robust.
void transmitImageFrame(pcap_t* handle, const vector<uint8_t>& imageData, uint32_t frameNumber, int repeatsPerChunk, bool confirmMode) {
    uint16_t totalChunks = (imageData.size() + PAYLOAD_MAX_SIZE - 1) / PAYLOAD_MAX_SIZE;

    for (uint16_t chunk = 0; chunk < totalChunks; chunk++) {
        size_t offset    = chunk * PAYLOAD_MAX_SIZE;
        size_t chunkSize = min((size_t)PAYLOAD_MAX_SIZE, imageData.size() - offset);

        uint8_t flags = confirmMode ? FLAG_ACK_REQUESTED : 0;
        Packet pkt(frameNumber, chunk, totalChunks, imageData.data() + offset, (uint16_t)chunkSize, flags);

        uint8_t buf[sizeof(PacketHeader) + PAYLOAD_MAX_SIZE];
        pkt.serialize(buf, sizeof(buf));

        int attempts = confirmMode ? (ACK_MAX_RETRIES + 1) : repeatsPerChunk;
        bool delivered = false;
        for (int r = 0; r < attempts; r++) {
            if (injectPacket(handle, buf, pkt.packetSize)) {
                if (r == 0) {
                    cout << "Sent frame=" << frameNumber
                         << " chunk=" << chunk << "/" << totalChunks
                         << " size=" << pkt.packetSize << "\n";
                }

                if (confirmMode) {
                    if (waitForAck(handle, frameNumber, chunk, ACK_TIMEOUT_MS)) {
                        delivered = true;
                        break;
                    }
                }
            }

            if (!confirmMode) usleep(1000); // 1ms between retransmissions
        }

        if (confirmMode && !delivered) {
            return;
        }
    }
}

void transmitEncodedBuffer(pcap_t* handle, const uint8_t* data, size_t size, uint32_t frameNumber, bool confirmMode) {
    if (!handle || !data || size == 0) return;

    uint16_t totalChunks = static_cast<uint16_t>((size + PAYLOAD_MAX_SIZE - 1) / PAYLOAD_MAX_SIZE);

    // Inter-chunk pacing: spread chunk injection over 70% of the frame interval
    // so chunks arrive steadily rather than in a burst, without adding full-frame delay.
    const uint32_t FRAME_BUDGET_US = (1000000 / CAMERA_FRAMERATE) * 70 / 100;
    uint32_t paceUs = (totalChunks > 1) ? (FRAME_BUDGET_US / totalChunks) : 0;

    for (uint16_t chunk = 0; chunk < totalChunks; chunk++) {
        size_t offset = static_cast<size_t>(chunk) * PAYLOAD_MAX_SIZE;
        size_t chunkSize = min(static_cast<size_t>(PAYLOAD_MAX_SIZE), size - offset);

        uint8_t flags = FLAG_IS_H264;
        if (confirmMode) flags |= FLAG_ACK_REQUESTED;
        Packet pkt(frameNumber, chunk, totalChunks, data + offset, static_cast<uint16_t>(chunkSize), flags);
        uint8_t buf[sizeof(PacketHeader) + PAYLOAD_MAX_SIZE];
        pkt.serialize(buf, sizeof(buf));

        if (!confirmMode) {
            injectPacket(handle, buf, pkt.packetSize);
            if (paceUs > 0) usleep(paceUs);
            continue;
        }

        bool delivered = false;
        for (int attempt = 0; attempt <= ACK_MAX_RETRIES; attempt++) {
            if (!injectPacket(handle, buf, pkt.packetSize)) continue;
            if (waitForAck(handle, frameNumber, chunk, ACK_TIMEOUT_MS)) {
                delivered = true;
                break;
            }
        }

        if (!delivered) {
            return;
        }
    }
}

int main(int argc, char** argv) {
    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);

    const char* iface = NETWORK_INTERFACE;
    char errbuf[PCAP_ERRBUF_SIZE];

    pcap_t* handle = pcap_open_live(iface, 2048, 1, 1, errbuf);
    if (!handle) {
        cerr << "Failed to open interface " << iface << ": " << errbuf << "\n";
        return 1;
    }
    global_handle = handle;

    bool useTestcard = false;
    bool confirmMode = false;  // default startup: low-latency mode
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--testcard") useTestcard = true;
        else if (arg == "--confirm" || arg == "--quality") confirmMode = true;
        else if (arg == "--no-confirm" || arg == "--latency") confirmMode = false;
    }

    cout << "VTX transmitting on " << iface << " — Ctrl+C to stop\n";
    cout << "Confirm mode: " << (confirmMode ? "ON" : "OFF") << "\n";

    if (useTestcard) {
        const string testImagePath = "media/testcard.jpg";
        vector<uint8_t> testImage;
        if (!readBinaryFile(testImagePath, testImage)) {
            pcap_close(handle);
            global_handle = nullptr;
            return 1;
        }

        cout << "Mode: testcard\n";
        cout << "Loaded " << testImagePath << " (" << testImage.size() << " bytes)\n";

        uint32_t frameNumber = 1;
        const int repeatsPerChunk = 2;
        while (running) {
            transmitImageFrame(handle, testImage, frameNumber++, repeatsPerChunk, confirmMode);
            usleep(150000);
        }
    } else {
        cout << "Mode: camera+hardware-encoder\n";
        CameraApp app;
        app.setEncodedPacketCallback([handle, confirmMode](const uint8_t* data, size_t size, uint64_t /*timestamp*/) {
            uint32_t frameNo = encodedFrameCounter.fetch_add(1);
            transmitEncodedBuffer(handle, data, size, frameNo, confirmMode);
        });

        int ret = app.run();
        pcap_close(handle);
        global_handle = nullptr;
        return ret;
    }

    pcap_close(handle);
    global_handle = nullptr;
    return 0;
}