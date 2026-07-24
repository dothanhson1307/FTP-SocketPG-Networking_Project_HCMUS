#pragma once
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include "RDTHeader.h"

std::vector<RDTPacket> splitPacketsInFile(const std::string& filepath);

void rdt_send(int client_fd, const std::string& filepath, const sockaddr* des, socklen_t deslen);

void rdt_recv(int sockfd, const std::string& output_filepath);
