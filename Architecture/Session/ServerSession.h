#pragma once

#include <filesystem>
#include <netinet/in.h>
#include <string>

using std::string;

struct ServerSession {
    string username;
    bool usernameAccepted = false;
    bool loggedIn = false;
    bool quitRequested = false;

    std::filesystem::path homeDir;
    std::filesystem::path currentDir;

    string clientIP = "";
    sockaddr_in clientAddress{};
    int clientFd = -1;

    //PORT va PASV tac dong
    bool isPassiveMode = false;
    string dataIp = "";
    int dataPort = -1;
};
