#pragma once
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include "RDTHeader.h"

using std::string;

std::vector<RDTPacket> splitPacketsInFile(const string& filepath);

void rdt_send(int client_fd, const string& filepath, const sockaddr* des, socklen_t deslen);

void rdt_recv(int sockfd, const string& output_filepath);
