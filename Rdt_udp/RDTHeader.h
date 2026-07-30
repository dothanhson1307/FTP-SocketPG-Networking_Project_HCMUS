#pragma once
#include <cstdint>
#include <cstddef>

constexpr uint8_t FLAG_DATA  = 0x01;
constexpr uint8_t FLAG_ACK   = 0x02;
constexpr uint8_t FLAG_START = 0x04;
constexpr uint8_t FLAG_END   = 0x08;

constexpr size_t MAX_PAYLOAD_SIZE = 1024;

#pragma pack(push, 1)
struct RDTHeader {
    uint32_t seq_num;   // Sequence number
    uint32_t ack_num;   // ACK sequence number
    uint16_t data_len;  // Length of payload in data[]
    uint16_t checksum;  
    uint8_t  flags;     // Packet type flags
};

struct RDTPacket {
    RDTHeader header;
    char data[MAX_PAYLOAD_SIZE];
};
#pragma pack(pop)

inline uint16_t calculateChecksum(const void* data, size_t len) {
    const uint16_t* buf = static_cast<const uint16_t*>(data);
    uint32_t sum = 0;

    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }

    if (len > 0) {
        sum += *reinterpret_cast<const uint8_t*>(buf);
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return static_cast<uint16_t>(~sum);
}

inline bool verifyChecksum(const RDTPacket& pkt) {
    RDTPacket temp = pkt;
    uint16_t received_checksum = temp.header.checksum;
    temp.header.checksum = 0;
    uint16_t calculated = calculateChecksum(&temp, sizeof(RDTHeader) + temp.header.data_len);
    return calculated == received_checksum;
}

