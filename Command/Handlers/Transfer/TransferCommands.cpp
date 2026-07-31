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

    sockaddr_in clientUdpAddr = session.clientAddress;
    clientUdpAddr.sin_port = htons(kDataPort);

    rdtSendFile(
        fullPath.string(),
        clientUdpAddr,
        sizeof(clientUdpAddr)
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

    rdtReceiveFile(fullPath.string());

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
