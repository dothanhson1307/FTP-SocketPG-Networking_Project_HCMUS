#ifndef RDT_H
#define RDT_H

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

void rdtReceiveFile(string filename, int port, const string& allowedIp = "");

void rdtSendFile(string filename, sockaddr_in server_addr, socklen_t addr_len);

#endif
