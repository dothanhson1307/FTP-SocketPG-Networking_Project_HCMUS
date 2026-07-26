#pragma once

#include <string>

inline constexpr int CLIENT_CONTROL_PORT = 8080;
inline constexpr int CLIENT_BUFFER_SIZE = 1024;

bool sendAll(int socketFd, const std::string& message);

bool receiveLine(
    int socketFd,
    std::string& pendingData,
    std::string& response
);
