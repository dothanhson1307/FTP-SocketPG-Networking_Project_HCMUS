#include "RDT.h"

uint16_t compute_checksum(const void *data, size_t len) {
    return calculateChecksum(data, len);
}

void rdtReceiveFile(string filename,int port,const string& allowedIp,const bool& isAppend) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return;
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{}, client_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (::bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return;
    }

    if(!allowedIp.empty()) {
        sockaddr_in expected_peer{};
        expected_peer.sin_family = AF_INET;
        inet_pton(AF_INET, allowedIp.c_str(), &expected_peer.sin_addr);
        connect(sockfd, (struct sockaddr*)&expected_peer, sizeof(expected_peer));
    }
    std::ios::openmode openMode = std::ios::binary | ((isAppend) ? std::ios::app  : std::ios::trunc);
    ofstream file(filename, openMode);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        close(sockfd);
        return;
    }

    cout << "[RDT Receiver] Listening for file payload on port "<< port << endl;

    socklen_t addr_len = sizeof(client_addr);
    char buffer[sizeof(RDTHeader) + MAX_PAYLOAD_SIZE];
    uint32_t expected_seq = 0;

    while (true) {
        ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                          (struct sockaddr*)&client_addr, &addr_len);
        if (bytes_received <= 0) break;

        RDTHeader* header = (RDTHeader*)buffer;
        char* payload = buffer + sizeof(RDTHeader);
        uint16_t received_checksum = header->checksum;
        header->checksum = 0;

        if (calculateChecksum(buffer, bytes_received) != received_checksum) {
            cout << "[RDT Receiver] Corrupted packet ignored." << endl;
            continue;
        }

        if (header->flags & FLAG_END) {
            cout << "[RDT Receiver] Transfer complete." << endl;
            RDTHeader ack_pkt{};
            ack_pkt.seq_num = 0;
            ack_pkt.ack_num = header->seq_num;
            ack_pkt.flags = FLAG_ACK | FLAG_END;
            ack_pkt.checksum = 0;
            ack_pkt.checksum = calculateChecksum(&ack_pkt, sizeof(RDTHeader));
            sendto(sockfd, &ack_pkt, sizeof(RDTHeader), 0, (struct sockaddr*)&client_addr, addr_len);
            break;
        }

        if (header->seq_num == expected_seq) {
            file.write(payload, header->data_len);
            expected_seq++;
        }

        RDTHeader ack_pkt{};
        ack_pkt.seq_num = 0;
        ack_pkt.ack_num = header->seq_num;
        ack_pkt.flags = FLAG_ACK;
        ack_pkt.checksum = 0;
        ack_pkt.checksum = calculateChecksum(&ack_pkt, sizeof(RDTHeader));

        sendto(sockfd, &ack_pkt, sizeof(RDTHeader), 0, (struct sockaddr*)&client_addr, addr_len);
    }

    file.close();
    close(sockfd);
}

void rdtSendFile(string filename, sockaddr_in server_addr, socklen_t addr_len) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return;
    }

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        close(sockfd);
        return;
    }

    char data_buffer[MAX_PAYLOAD_SIZE];
    uint32_t seq_num = 0;

    while (file.read(data_buffer, sizeof(data_buffer)) || file.gcount() > 0) {
        size_t bytes_read = file.gcount();

        char packet_buffer[sizeof(RDTHeader) + MAX_PAYLOAD_SIZE];
        RDTHeader* header = (RDTHeader*)packet_buffer;

        header->seq_num = seq_num;
        header->ack_num = 0;
        header->data_len = bytes_read;
        header->flags = FLAG_DATA;
        header->checksum = 0;

        memcpy(packet_buffer + sizeof(RDTHeader), data_buffer, bytes_read);
        header->checksum = calculateChecksum(packet_buffer, sizeof(RDTHeader) + bytes_read);

        bool ack_received = false;
        while (!ack_received) {
            sendto(sockfd, packet_buffer, sizeof(RDTHeader) + bytes_read, 0,
                   (struct sockaddr*)&server_addr, addr_len);

            char ack_buffer[sizeof(RDTHeader)];
            ssize_t bytes_recvd = recvfrom(sockfd, ack_buffer, sizeof(ack_buffer), 0, NULL, NULL);

            if (bytes_recvd >= (ssize_t)sizeof(RDTHeader)) {
                RDTHeader* ack_hdr = (RDTHeader*)ack_buffer;
                if ((ack_hdr->flags & FLAG_ACK) && ack_hdr->ack_num == seq_num) {
                    ack_received = true;
                    seq_num++;
                }
            }
        }
    }

    RDTHeader fin_hdr{};
    fin_hdr.seq_num = seq_num;
    fin_hdr.flags = FLAG_END;
    fin_hdr.checksum = 0;
    fin_hdr.checksum = calculateChecksum(&fin_hdr, sizeof(RDTHeader));

    sendto(sockfd, &fin_hdr, sizeof(RDTHeader), 0, (struct sockaddr*)&server_addr, addr_len);

    file.close();
    close(sockfd);
}
