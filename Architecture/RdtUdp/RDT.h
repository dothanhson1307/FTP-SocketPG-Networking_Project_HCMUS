#ifndef RDT_H
#define RDT_H

#include <atomic>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "RDTHeader.h"

using namespace std;

uint16_t compute_checksum(const void *data, size_t len);

void rdtReceiveFile(string filename, int port, const string& allowedIp = "", const bool& isAppend = false, std::atomic_bool* isTransferring = nullptr, std::atomic_bool* abortRequested = nullptr, char transferMode = 'S',int client_fd = -1);

void rdtSendFile(string filename, sockaddr_in server_addr, socklen_t addr_len, std::atomic_bool* isTransferring = nullptr, std::atomic_bool* abortRequested = nullptr, char transferMode = 'S',int client_fd = -1);

// RLE Compression / Decompression helpers for MODE C
size_t compressRLE(const char* input, size_t inputLen, char* output, size_t maxOutputLen);
size_t decompressRLE(const char* input, size_t inputLen, char* output, size_t maxOutputLen);

#endif



