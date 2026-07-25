#pragma once
#include <string>
#include <utility>
#include <cstddef>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>

using std::string;

void sendResponse(int client_fd, const string& response);
std::pair<string, string> returnResponse(int client_fd, char* buffer, size_t buffer_capacity);
