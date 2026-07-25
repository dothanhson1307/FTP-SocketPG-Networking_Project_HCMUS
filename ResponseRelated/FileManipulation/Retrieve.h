#pragma once
#include <string>
#include <netinet/in.h>

using std::string;

void retrieve(const string& filepath, sockaddr_in client_addr, socklen_t client_len);