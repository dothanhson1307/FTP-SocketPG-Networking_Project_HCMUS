#pragma once

#include <filesystem>
#include <netinet/in.h>
#include <string>

using std::string;

struct ClientData {
    string username;
    bool usernameAccepted = false;
    bool loggedIn = false;
    bool quitRequested = false;

    std::filesystem::path homeDir;
    std::filesystem::path currentDir;

    // runtime data, set by Server.cpp after accept(...)
    sockaddr_in clientAddress{};
    int clientFd = -1;
};
