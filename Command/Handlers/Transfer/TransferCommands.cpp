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

string handleRetr(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        return ftpInvalidArguments();
    }

    if (!isLoggedIn(session)) {
        return ftpNotLoggedIn();
    }

    std::filesystem::path filename = std::filesystem::path(args[1]).filename();
    std::filesystem::path fullPath = session.homeDir / session.currentDir / filename;

    std::error_code error;
    if (!std::filesystem::is_regular_file(fullPath, error) || error) {
        return ftpFileUnavailable();
    }

    if (!sendSessionReply(session, ftpOpeningDataConnection("RETR"))) {
        return "";
    }

    int targetPort = (session.dataPort > 0) ? session.dataPort : kDataPort;
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

    return ftpTransferComplete();
}

string handleStor(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        return ftpInvalidArguments();
    }

    if (!isLoggedIn(session)) {
        return ftpNotLoggedIn();
    }

    std::filesystem::path filename = std::filesystem::path(args[1]).filename();
    std::filesystem::path fullPath = session.homeDir / session.currentDir / filename;

    if (!sendSessionReply(session, ftpOpeningDataConnection("STOR"))) {
        return "";
    }

    int listenPort = (session.dataPort > 0) ? session.dataPort : kDataPort;
    char clientIp[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &session.clientAddress.sin_addr, clientIp, sizeof(clientIp));

    rdtReceiveFile(fullPath.string(), listenPort, clientIp);

    return ftpTransferComplete();
}

string handleHash(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        return ftpInvalidArguments();
    }

    if (!isLoggedIn(session)) {
        return ftpNotLoggedIn();
    }

    std::filesystem::path filename = std::filesystem::path(args[1]).filename();
    std::filesystem::path fullPath = session.homeDir / session.currentDir / filename;

    string hashValue = calculateFileSHA256(fullPath.string());
    if (hashValue.empty()) {
        return ftpFileUnavailable();
    }

    return ftpSha256(hashValue);
}
