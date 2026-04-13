#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "config.h"
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <algorithm>

constexpr uint16_t MAGIC_NUMBER = 0x5B1E;   // SABLE because i have an ego
constexpr uint8_t PROTOCOL_VERSION = 1;     

// Flags bitfield
constexpr uint8_t FLAG_ACK_REQUESTED = 0x01;
constexpr uint8_t FLAG_IS_ACK        = 0x02;

#pragma pack(push, 1)
class PacketHeader {
public:
    uint16_t magic;         // Magic number for validation
    uint8_t  version;       // Protocol version for compatibility
    uint8_t  flags;         // Bitfield for flags (e.g., ACK requested, is ACK, etc.)
    uint32_t framecounter;  // Frame counter for tracking
    uint16_t chunknumber;   // Chunk number within the frame
    uint16_t totalchunks;   // Total chunks for this frame
    uint16_t headersize;   // Size of the header in bytes
    uint32_t timestamp;     // Timestamp for the packet

    PacketHeader()
        : magic(MAGIC_NUMBER), version(PROTOCOL_VERSION), flags(0),
          framecounter(0), chunknumber(0), totalchunks(0), headersize(0), timestamp(0) {}

    bool isValid() const {
        return magic == MAGIC_NUMBER && version == PROTOCOL_VERSION;
    }
};
#pragma pack(pop)

class Packet {
public:

    PacketHeader header;
    uint8_t payload[PAYLOAD_MAX_SIZE];  // Payload buffer

    uint16_t payloadSize;                // Actual size of the payload data
    uint16_t packetSize;                 // Total size of the packet (header + payload)

    Packet();
    Packet(uint32_t framecounter, uint16_t chunknumber, uint16_t totalchunks, 
           const uint8_t* data, uint16_t dataSize, uint8_t flags = 0);

    // Serialize the packet into a byte buffer for transmission
    void serialize(uint8_t* buffer, size_t bufferSize);
};

#endif // PROTOCOL_H
