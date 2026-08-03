#pragma once

#include "Client/Client.h"

#include <netinet/in.h>
#include <string>

bool handleTransferCommand(
    int clientFd,
    const sockaddr_in& serverAddress,
    std::string& pendingData,
    const ClientSession& session,
    const std::string& rawLine
);


