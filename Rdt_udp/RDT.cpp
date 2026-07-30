#include "RDT.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>

using std::string;

std::vector<RDTPacket> splitPacketsInFile(const string& filepath) {
    std::vector<RDTPacket> packets;
    std::ifstream fin(filepath, std::ios::binary);
    if (!fin.is_open()) {
        std::cerr << "Can't open file: " << filepath << "\n";
        return packets;
    }
    uint32_t seq_num = 1;
    while (fin) {
        RDTPacket pkt{};
        fin.read(pkt.data, MAX_PAYLOAD_SIZE);
        if (fin.gcount() == 0) break;
        pkt.header.seq_num = seq_num++;
        pkt.header.ack_num = 0;
        pkt.header.data_len = static_cast<uint16_t>(fin.gcount());
        pkt.header.flags = FLAG_DATA;
        pkt.header.checksum = 0;
        pkt.header.checksum = calculateChecksum(&pkt, sizeof(RDTHeader) + pkt.header.data_len);
        packets.push_back(pkt);
    }
    if (!packets.empty()) {
        packets.back().header.flags |= FLAG_END;
        packets.back().header.checksum = 0;
        packets.back().header.checksum = calculateChecksum(&packets.back(), sizeof(RDTHeader) + packets.back().header.data_len);
    }
    fin.close();
    return packets;
}

void rdt_send(int client_fd, const string& filepath, const sockaddr* des, socklen_t deslen) {
    std::vector<RDTPacket> packets = splitPacketsInFile(filepath);
    struct timeval tv{ .tv_sec = 2, .tv_usec = 0 };
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    for (const RDTPacket& packet : packets) {
        bool acked = false;
        while (!acked) {
            sendto(client_fd, &packet, sizeof(packet.header) + packet.header.data_len, 0, des, deslen);
            RDTPacket recieved_packet{};
            sockaddr_in from_addrs{};
            socklen_t from_len = sizeof(from_addrs);
            ssize_t bytes_received = recvfrom(client_fd, &recieved_packet, sizeof(recieved_packet), 0, reinterpret_cast<sockaddr*>(&from_addrs), &from_len);
            if (bytes_received > 0) {
                if (verifyChecksum(recieved_packet) && 
                   (recieved_packet.header.flags & FLAG_ACK) && 
                    recieved_packet.header.ack_num == packet.header.seq_num) {
                    acked = true;
                }
            }
        }
    }
}

void rdt_recv(int sockfd, const string& output_filepath) {
    std::ofstream fout(output_filepath, std::ios::binary);
    if (!fout.is_open()) {
        std::cerr << "Failed to open output file: " << output_filepath << "\n";
        return;
    }
    uint32_t expected_seq = 1;
    while (true) {
        RDTPacket pkt{};
        sockaddr_in sender_addr{};
        socklen_t sender_len = sizeof(sender_addr);
        ssize_t bytes_recvd = recvfrom(sockfd, &pkt, sizeof(pkt), 0, reinterpret_cast<sockaddr*>(&sender_addr), &sender_len);
        if (bytes_recvd <= 0) continue;

        if (!verifyChecksum(pkt)) continue;

        if (pkt.header.flags & FLAG_DATA) {
            if (pkt.header.seq_num == expected_seq) {
                fout.write(pkt.data, pkt.header.data_len);
                
                RDTPacket ack_pkt{};
                ack_pkt.header.seq_num = 0;
                ack_pkt.header.ack_num = pkt.header.seq_num;
                ack_pkt.header.data_len = 0;
                ack_pkt.header.flags = FLAG_ACK;
                ack_pkt.header.checksum = 0;
                ack_pkt.header.checksum = calculateChecksum(&ack_pkt, sizeof(RDTHeader));

                sendto(sockfd, &ack_pkt, sizeof(RDTHeader), 0, reinterpret_cast<const sockaddr*>(&sender_addr), sender_len);
                expected_seq++;

                if (pkt.header.flags & FLAG_END) break;
            }
            else if (pkt.header.seq_num < expected_seq) {
                RDTPacket ack_pkt{};
                ack_pkt.header.seq_num = 0;
                ack_pkt.header.ack_num = pkt.header.seq_num;
                ack_pkt.header.data_len = 0;
                ack_pkt.header.flags = FLAG_ACK;
                ack_pkt.header.checksum = 0;
                ack_pkt.header.checksum = calculateChecksum(&ack_pkt, sizeof(RDTHeader));

                sendto(sockfd, &ack_pkt, sizeof(RDTHeader), 0, reinterpret_cast<const sockaddr*>(&sender_addr), sender_len);
            }
        }
    }
    fout.close();
}
