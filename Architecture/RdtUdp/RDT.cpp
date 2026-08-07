#include "RDT.h"
#include "../../Helper/FtpReply.h"
#include "../../Helper/SocketIO.h"
#include <atomic>
#include <vector>

uint16_t compute_checksum(const void *data, size_t len) {
    return calculateChecksum(data, len);
}

size_t compressRLE(const char* input, size_t inputLen, char* output, size_t maxOutputLen) {
    size_t inIdx = 0;
    size_t outIdx = 0;

    while (inIdx < inputLen && outIdx < maxOutputLen) {
        unsigned char currentByte = static_cast<unsigned char>(input[inIdx]);
        size_t runLen = 1;

        while (inIdx + runLen < inputLen && runLen < 255 && input[inIdx + runLen] == input[inIdx]) {
            runLen++;
        }
        //breaks even when >=3
        if (runLen >= 3 || currentByte == 0xFF) {
            if (outIdx + 3 > maxOutputLen) break;
            output[outIdx++] = static_cast<char>(0xFF);
            output[outIdx++] = static_cast<char>(runLen);
            output[outIdx++] = static_cast<char>(currentByte);
            inIdx += runLen;
        } 
        else{//no compress
            for (size_t i = 0; i < runLen; ++i) {
                if (outIdx >= maxOutputLen) break;
                output[outIdx++] = static_cast<char>(currentByte);
                inIdx++;
            }
        }
    }

    return outIdx;
}

// RLE Decompression: Expands [0xFF][Count][Byte] escape sequences
size_t decompressRLE(const char* input, size_t inputLen, char* output, size_t maxOutputLen) {
    size_t inIdx = 0;
    size_t outIdx = 0;

    while (inIdx < inputLen && outIdx < maxOutputLen) {
        unsigned char currentByte = static_cast<unsigned char>(input[inIdx]);
        //OxFF means compressed bytes so transfer back else just convert it normally
        if (currentByte == 0xFF && inIdx + 2 < inputLen) {
            size_t count = static_cast<unsigned char>(input[inIdx + 1]);
            char val = input[inIdx + 2];
            inIdx += 3;

            for (size_t i = 0; i < count && outIdx < maxOutputLen; ++i) {
                output[outIdx++] = val;
            }
        } else {
            output[outIdx++] = static_cast<char>(currentByte);
            inIdx++;
        }
    }

    return outIdx;
}

void rdtReceiveFile(string filename, int port, const string& allowedIp, const bool& isAppend, std::atomic_bool* isTransferring, std::atomic_bool* abortRequested, char transferMode,int client_fd) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return;
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000; // 500ms timeout
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sockaddr_in server_addr{}, client_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (::bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return;
    }

    if (!allowedIp.empty()) {
        sockaddr_in expected_peer{};
        expected_peer.sin_family = AF_INET;
        inet_pton(AF_INET, allowedIp.c_str(), &expected_peer.sin_addr);
        connect(sockfd, (struct sockaddr*)&expected_peer, sizeof(expected_peer));
    }
    std::ios::openmode openMode = std::ios::binary | ((isAppend) ? std::ios::app : std::ios::trunc);
    ofstream file(filename, openMode);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        close(sockfd);
        return;
    }

    cout << "[RDT Receiver] Listening for file payload on port " << port << " (Mode: " << transferMode << ")" << endl;

    socklen_t addr_len = sizeof(client_addr);
    char buffer[sizeof(RDTHeader) + MAX_PAYLOAD_SIZE];
    uint32_t expected_seq = 0;

    if (isTransferring) isTransferring->store(true);
    if (abortRequested) abortRequested->store(false);

    while (true) {
        if ((abortRequested && abortRequested->load()) || (isTransferring && !isTransferring->load())) {
            cout << "[RDT Receiver] Transfer aborted by request." << endl;
            break;
        }

        ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                          (struct sockaddr*)&client_addr, &addr_len);
        if (bytes_received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            break;
        }
        if (bytes_received == 0) break;

        RDTHeader* header = (RDTHeader*)buffer;
        char* rawPayload = buffer + sizeof(RDTHeader);
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
            char decompBuffer[MAX_PAYLOAD_SIZE];
            const char* finalPayload = rawPayload;
            size_t finalLen = header->data_len;

            // Handle Mode C Decompression
            if (transferMode == 'C') {
                finalLen = decompressRLE(rawPayload, header->data_len, decompBuffer, sizeof(decompBuffer));
                finalPayload = decompBuffer;
            }
            // Handle Mode B Block Framing
            else if (transferMode == 'B' && header->data_len >= 3) {
                // Strip 3-byte block header [descriptor][len_hi][len_lo]
                finalPayload = rawPayload + 3;
                finalLen = header->data_len - 3;
            }

            file.write(finalPayload, finalLen);
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

    if (isTransferring) isTransferring->store(false);
    file.close();
    close(sockfd);

    if (client_fd >= 0) {
        if (abortRequested && abortRequested->load()) {
            sendAll(client_fd, ftpTransferAborted());
        } else {
            sendAll(client_fd, ftpTransferComplete());
        }
    }
}

void rdtSendFile(string filename, sockaddr_in server_addr, socklen_t addr_len, std::atomic_bool* isTransferring, std::atomic_bool* abortRequested, char transferMode,int client_fd) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return;
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000; // 500ms timeout
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        close(sockfd);
        return;
    }

    char data_buffer[MAX_PAYLOAD_SIZE];
    uint32_t seq_num = 0;

    if (isTransferring) isTransferring->store(true);
    if (abortRequested) abortRequested->store(false);

    while (file.read(data_buffer, sizeof(data_buffer)) || file.gcount() > 0) {
        if ((abortRequested && abortRequested->load()) || (isTransferring && !isTransferring->load())) {
            cout << "[RDT Sender] Transfer aborted by request." << endl;
            break;
        }

        size_t raw_bytes_read = file.gcount();
        char processed_payload[MAX_PAYLOAD_SIZE];
        const char* final_payload = data_buffer;
        size_t payload_len = raw_bytes_read;

        // Handle Mode C Compression
        if (transferMode == 'C') {
            payload_len = compressRLE(data_buffer, raw_bytes_read, processed_payload, sizeof(processed_payload));
            final_payload = processed_payload;
        }
        // Handle Mode B Block Framing
        else if (transferMode == 'B') {
            // Prepend 3-byte block header: [0x00 (normal block)][len_hi][len_lo]
            processed_payload[0] = 0x00;
            processed_payload[1] = static_cast<char>((raw_bytes_read >> 8) & 0xFF);
            processed_payload[2] = static_cast<char>(raw_bytes_read & 0xFF);
            memcpy(processed_payload + 3, data_buffer, raw_bytes_read);
            payload_len = raw_bytes_read + 3;
            final_payload = processed_payload;
        }

        char packet_buffer[sizeof(RDTHeader) + MAX_PAYLOAD_SIZE];
        RDTHeader* header = (RDTHeader*)packet_buffer;

        header->seq_num = seq_num;
        header->ack_num = 0;
        header->data_len = payload_len;
        header->flags = FLAG_DATA;
        header->checksum = 0;

        memcpy(packet_buffer + sizeof(RDTHeader), final_payload, payload_len);
        header->checksum = calculateChecksum(packet_buffer, sizeof(RDTHeader) + payload_len);

        bool ack_received = false;
        while (!ack_received) {
            if ((abortRequested && abortRequested->load()) || (isTransferring && !isTransferring->load())) {
                cout << "[RDT Sender] Transfer aborted while awaiting ACK." << endl;
                break;
            }

            sendto(sockfd, packet_buffer, sizeof(RDTHeader) + payload_len, 0,
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

        if ((abortRequested && abortRequested->load()) || (isTransferring && !isTransferring->load())) {
            break;
        }
    }

    if ((!abortRequested || !abortRequested->load()) && (!isTransferring || isTransferring->load())) {
        RDTHeader fin_hdr{};
        fin_hdr.seq_num = seq_num;
        fin_hdr.flags = FLAG_END;
        fin_hdr.checksum = 0;
        fin_hdr.checksum = calculateChecksum(&fin_hdr, sizeof(RDTHeader));

        sendto(sockfd, &fin_hdr, sizeof(RDTHeader), 0, (struct sockaddr*)&server_addr, addr_len);
    }

    if (isTransferring) isTransferring->store(false);
    file.close();
    close(sockfd);

    if (client_fd >= 0) {
        if (abortRequested && abortRequested->load()) {
            sendAll(client_fd, ftpTransferAborted());
        } else {
            sendAll(client_fd, ftpTransferComplete());
        }
    }
}
