#pragma once

#include <string>

inline constexpr int SERVER_CONTROL_PORT = 8080;
inline constexpr int SERVER_BUFFER_SIZE = 1024;

bool sendAll(int socketFd, const std::string& message);
