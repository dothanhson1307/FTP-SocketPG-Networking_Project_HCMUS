#pragma once

#include <netinet/in.h>
#include <string>

using std::string;

struct ClientSession;

// could be declared with constexpr to reduce time complexity
const int kDataPort = 8081;

bool handleTransferCommand(
    int clientFd,
    const sockaddr_in& serverAddress,
    string& pendingData,
    const ClientSession& session,
    const string& command
);
