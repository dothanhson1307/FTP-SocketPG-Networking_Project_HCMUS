#pragma once

#include "Client/Client.h"
#include <string>
#include <vector>

std::vector<std::string> tokenizeCommand(const std::string& command);

bool startsWithReplyCode(const std::string& response, const std::string& code);

void updateSessionAfterReply(
    const std::string& command,
    const std::string& response,
    ClientSession& session
);

bool receiveLine(
    int socketFd,
    std::string& pendingData,
    std::string& response
);
