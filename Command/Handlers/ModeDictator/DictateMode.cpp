#include "DictateMode.h"
#include "Helper/FtpReply.h"

#include <arpa/inet.h>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace {

bool isNumeric(const std::string& str) {
    if (str.empty()) return false;
    return std::all_of(str.begin(), str.end(), [](unsigned char c) {
        return std::isdigit(c);
    });
}

}

std::string handleActiveMode(const std::vector<std::string>& args, ServerSession& session) {
    if (args.size() != 2) {
        return ftpInvalidArguments();
    }
    if (!session.loggedIn) {
        return ftpNotLoggedIn();
    }

    std::stringstream ss(args[1]);
    std::string token;
    std::vector<int> parts;

    while (std::getline(ss, token, ',')) {
        if (!isNumeric(token)) {
            return ftpInvalidArguments();
        }
        try {
            int val = std::stoi(token);
            if (val < 0 || val > 255) {
                return ftpInvalidArguments();
            }
            parts.push_back(val);
        } catch (...) {
            return ftpInvalidArguments();
        }
    }

    if (parts.size() != 6) {
        return ftpInvalidArguments();
    }

    int port = parts[4] * 256 + parts[5];
    if (port <= 0 || port > 65535) {
        return ftpInvalidArguments();
    }

    session.dataIp = std::to_string(parts[0]) + "." + std::to_string(parts[1]) + "." +
                     std::to_string(parts[2]) + "." + std::to_string(parts[3]);
    session.dataPort = port;
    session.isPassiveMode = false;

    return ftpCommandSuccessful("PORT command successful.");
}

std::string handlePassiveMode(const std::vector<std::string>& args, ServerSession& session) {
    if (args.size() != 1) {
        return ftpInvalidArguments();
    }
    if (!session.loggedIn) {
        return ftpNotLoggedIn();
    }

    int passivePort = 8083;
    session.dataPort = passivePort;
    session.isPassiveMode = true;

    //default ip
    std::string serverIpStr = "127.0.0.1";
    if (session.clientFd >= 0) {
        sockaddr_in localAddr{};
        socklen_t addrLen = sizeof(localAddr);
        if (getsockname(session.clientFd, reinterpret_cast<sockaddr*>(&localAddr), &addrLen) == 0) {
            char ipBuf[INET_ADDRSTRLEN]{};
            if (inet_ntop(AF_INET, &localAddr.sin_addr, ipBuf, sizeof(ipBuf))) {
                std::string currentIp(ipBuf);
                if (currentIp != "0.0.0.0") {
                    serverIpStr = currentIp;
                }
            }
        }
    }

    // Format IP string from dots to commas(for sinaddr)
    std::string ipCommas = serverIpStr;
    std::replace(ipCommas.begin(), ipCommas.end(), '.', ',');

    int p1 = passivePort / 256;
    int p2 = passivePort % 256;

    return ftpPassiveMode(ipCommas, p1, p2);
}