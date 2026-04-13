#include "protocol.h"
#include <pcap.h>
#include <iostream>
#include <algorithm>
#include "config.h"

Packet::Packet() {
    // Initialize payload to zero
    std::fill(payload, payload + PAYLOAD_MAX_SIZE, 0);
}

Packet::Packet(uint32_t framecounter, uint16_t chunknumber, uint16_t totalchunks, 
           const uint8_t* data, uint16_t dataSize, uint8_t flags)

    : header(), payloadSize(dataSize), packetSize(0) {

        header.framecounter = framecounter;
        header.chunknumber = chunknumber;
        header.totalchunks = totalchunks;
        header.headersize = sizeof(PacketHeader);
    std::memcpy(payload, data, dataSize);
}

void Packet::serialize(uint8_t* buffer, size_t bufferSize) {
    if (bufferSize < sizeof(PacketHeader) + payloadSize) {
        // Not enough space to serialize
        return;
    }
    
    // Copy header
    std::memcpy(buffer, &header, sizeof(PacketHeader));
    
    // Copy payload
    std::memcpy(buffer + sizeof(PacketHeader), payload, payloadSize);
    
    // Set total packet size
    packetSize = sizeof(PacketHeader) + payloadSize;
}

// Helper function to chunk, serialize, and transmit encoded data
void transmitEncodedFrame(const uint8_t* encodedData, size_t encodedSize, uint32_t frameNumber, pcap_t* handle) {
    constexpr size_t MAX_PAYLOAD_SIZE = PAYLOAD_MAX_SIZE;
    uint16_t totalChunks = (encodedSize + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE;

    for (uint16_t chunkNum = 0; chunkNum < totalChunks; ++chunkNum) {
        size_t offset = chunkNum * MAX_PAYLOAD_SIZE;
        size_t chunkSize = std::min(MAX_PAYLOAD_SIZE, encodedSize - offset);

        Packet pkt(frameNumber, chunkNum, totalChunks, encodedData + offset, chunkSize, /*flags=*/0);
        uint8_t buffer[sizeof(PacketHeader) + MAX_PAYLOAD_SIZE];
        pkt.serialize(buffer, sizeof(buffer));

        // Transmit the packet
        int ret = pcap_inject(handle, buffer, pkt.packetSize);
        if (ret == -1) {
            std::cerr << "Error injecting packet: " << pcap_geterr(handle) << std::endl;
        }
    }
}