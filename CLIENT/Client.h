#pragma once

#include <atomic>
#include <filesystem>
#include <string>

using std::string;

inline constexpr int CLIENT_CONTROL_PORT = 8080;
inline constexpr int CLIENT_BUFFER_SIZE = 1024;

// This is local client state only. It lets STOR find files in the folder of
// the user that has successfully logged in; it is not sent to the server.
struct ClientSession {
    string username;
    bool usernameAccepted = false;
    bool loggedIn = false;
    std::filesystem::path currentDir = ".";

    // Data channel settings negotiated with server (PASV / PORT)
    bool isPassiveMode = false;
    int dataPort = 8081;
    char transferMode = 'S';

    std::atomic_bool isTransferring{false};
    std::atomic_bool abortRequested{false};

    void reset() {
        username.clear();
        usernameAccepted = false;
        loggedIn = false;
        currentDir = ".";
        isPassiveMode = false;
        dataPort = 8081;
        transferMode = 'S';
        isTransferring.store(false);
        abortRequested.store(false);
    }
};



bool receiveLine(
    int socketFd,
    string& pendingData,
    string& response
);
