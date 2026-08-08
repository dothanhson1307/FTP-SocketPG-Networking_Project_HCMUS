#pragma once

#include <filesystem>
#include <netinet/in.h>
#include <string>
#include <atomic>
#include <shared_mutex>

using std::string;

struct ServerSession {
    mutable std::shared_mutex sessionMutex;

    string username;

    bool usernameAccepted = false;
    bool loggedIn = false;
    bool quitRequested = false;

    std::filesystem::path homeDir;
    std::filesystem::path currentDir;

    string clientIP = "";
    sockaddr_in clientAddress{};
    int clientFd = -1;

    // Data Channel Mode & Type state
    bool isPassiveMode = false;
    string dataIp = "";
    int dataPort = -1;
    char transferType = 'I'; 
    char transferMode = 'S'; 

    std::atomic_bool isTransferring{false};
    std::atomic_bool abortRequested{false};

    // when client sent RNFR, renamePending turn true - Waiting for RNTO
    // if RNTO happend before RNFR --> error
    bool renamePending = false;
    std::filesystem::path renameSourcePath;
};

