#include "DictateMode.h"
#include "Helper/FtpReply.h"
#include "Helper/SocketIO.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
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

int findAvailablePort() {
    int tempFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (tempFd < 0) return 8083;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(0);

    if (bind(tempFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(tempFd);
        return 8083;
    }

    socklen_t len = sizeof(addr);
    int assignedPort = 8083;
    if (getsockname(tempFd, reinterpret_cast<sockaddr*>(&addr), &len) == 0) {
        assignedPort = ntohs(addr.sin_port);
    }

    close(tempFd);
    return assignedPort;
}

}

void handleActiveMode(const std::vector<std::string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }
    if (!session.loggedIn) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    std::stringstream ss(args[1]);
    std::string token;
    std::vector<int> parts;

    while (std::getline(ss, token, ',')) {
        if (!isNumeric(token)) {
            sendAll(session.clientFd, ftpInvalidArguments());
            return;
        }
        try {
            int val = std::stoi(token);
            if (val < 0 || val > 255) {
                sendAll(session.clientFd, ftpInvalidArguments());
                return;
            }
            parts.push_back(val);
        } catch (...) {
            sendAll(session.clientFd, ftpInvalidArguments());
            return;
        }
    }

    if (parts.size() != 6) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    int port = parts[4] * 256 + parts[5];
    if (port <= 0 || port > 65535) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    session.dataIp = std::to_string(parts[0]) + "." + std::to_string(parts[1]) + "." +
                     std::to_string(parts[2]) + "." + std::to_string(parts[3]);
    session.dataPort = port;
    session.isPassiveMode = false;

    sendAll(session.clientFd, ftpCommandSuccessful("PORT command successful."));
}

void handlePassiveMode(const std::vector<std::string>& args, ServerSession& session) {
    if (args.size() != 1) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }
    if (!session.loggedIn) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    int passivePort = findAvailablePort();
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

    sendAll(session.clientFd, ftpPassiveMode(ipCommas, p1, p2));
}