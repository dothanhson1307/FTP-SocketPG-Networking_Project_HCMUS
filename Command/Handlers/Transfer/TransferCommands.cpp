#include "TransferCommands.h"

#include "Architecture/RdtUdp/RDT.h"
#include "Command/Integrity/Hash.h"
#include "Server/Server.h"
#include "Helper/FtpReply.h"
#include "Helper/SocketIO.h"

#include <arpa/inet.h>
#include <filesystem>
#include <sys/socket.h>
#include <unistd.h>

using std::string;

static bool isLoggedIn(const ServerSession& session) {
    return session.loggedIn && !session.homeDir.empty();
}

static bool sendSessionReply(const ServerSession& session, const string& reply) {
    return sendAll(session.clientFd, reply);
}

void handleRetr(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    std::filesystem::path filename = std::filesystem::path(args[1]).filename();
    std::filesystem::path fullPath = session.homeDir / session.currentDir / filename;

    std::error_code error;
    if (!std::filesystem::is_regular_file(fullPath, error) || error) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    if (!sendSessionReply(session, ftpOpeningDataConnection("RETR"))) {
        return;
    }

    int targetPort = (session.dataPort > 0) ? session.dataPort : 8081;
    sockaddr_in targetUdpAddr = session.clientAddress;

    if (!session.isPassiveMode && !session.dataIp.empty()) {
        inet_pton(AF_INET, session.dataIp.c_str(), &targetUdpAddr.sin_addr);
    }
    targetUdpAddr.sin_port = htons(targetPort);

    rdtSendFile(
        fullPath.string(),
        targetUdpAddr,
        sizeof(targetUdpAddr)
    );

    sendAll(session.clientFd, ftpTransferComplete());
}

void handleStor(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    std::filesystem::path filename = std::filesystem::path(args[1]).filename();
    std::filesystem::path fullPath = session.homeDir / session.currentDir / filename;

    if (!sendSessionReply(session, ftpOpeningDataConnection("STOR"))) {
        return;
    }

    int listenPort = (session.dataPort > 0) ? session.dataPort : 8081;
    char clientIp[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &session.clientAddress.sin_addr, clientIp, sizeof(clientIp));

    rdtReceiveFile(fullPath.string(), listenPort, clientIp);

    sendAll(session.clientFd, ftpTransferComplete());
}

void handleAppe(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }
    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }
    std::filesystem::path filename = std::filesystem::path(args[1]).filename();
    std::filesystem::path fullPath = std::filesystem::path("Repository/server_data/Appendables") / filename;

    if (!sendSessionReply(session, ftpOpeningDataConnection("APPE"))) {
        return;
    }
    int listenPort = (session.dataPort > 0) ? session.dataPort : 8081;
    char clientIp[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &session.clientAddress.sin_addr, clientIp, sizeof(clientIp));

    rdtReceiveFile(fullPath.string(), listenPort, clientIp, true);

    sendAll(session.clientFd, ftpTransferComplete());
}

// void handleAbort(const std::vector<string>& args, ServerSession& session) {
//     if (!isLoggedIn(session)) {
//         sendAll(session.clientFd, ftpNotLoggedIn());
//         return;
//     }
//     sendAll(session.clientFd, ftpTransferAborted());
// }

void handleHash(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    std::filesystem::path filename = std::filesystem::path(args[1]).filename();
    std::filesystem::path fullPath = session.homeDir / session.currentDir / filename;

    string hashValue = calculateFileSHA256(fullPath.string());
    if (hashValue.empty()) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    sendAll(session.clientFd, ftpSha256(hashValue));
}
