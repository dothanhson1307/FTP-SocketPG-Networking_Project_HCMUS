#include "TransferCommands.h"

#include "Architecture/RdtUdp/RDT.h"
#include "Server/Server.h"
#include "Helper/FtpReply.h"
#include "Helper/SocketIO.h"

#include <thread>
#include <arpa/inet.h>
#include <filesystem>
#include <shared_mutex>
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

    if (session.isTransferring.load()) {
        sendAll(session.clientFd, ftpTransferAlreadyInProgress());
        return;
    }

    std::filesystem::path fullPath;
    sockaddr_in targetUdpAddr{};
    char mode = 'S';

    {
        std::shared_lock<std::shared_mutex> lock(session.sessionMutex);
        if (!isLoggedIn(session)) {
            sendAll(session.clientFd, ftpNotLoggedIn());
            return;
        }

        std::filesystem::path filename = std::filesystem::path(args[1]).filename();
        fullPath = std::filesystem::path("Repository/server_data/Downloadable_files") / filename;


        int targetPort = (session.dataPort > 0) ? session.dataPort : 8081;
        targetUdpAddr = session.clientAddress;
        mode = session.transferMode;

        if (!session.isPassiveMode && !session.dataIp.empty()) {
            inet_pton(AF_INET, session.dataIp.c_str(), &targetUdpAddr.sin_addr);
        }
        targetUdpAddr.sin_port = htons(targetPort);
    }

    std::error_code error;
    if (!std::filesystem::is_regular_file(fullPath, error) || error) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }


    if (!sendSessionReply(session, ftpOpeningDataConnection("RETR"))) {
        return;
    }
    std::thread transferThread(
        rdtSendFile,
        fullPath.string(),
        targetUdpAddr,
        sizeof(targetUdpAddr),
        &session.isTransferring,
        &session.abortRequested,
        mode,session.clientFd
    );
    transferThread.detach();
}

void handleStor(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (session.isTransferring.load()) {
        sendAll(session.clientFd, ftpTransferAlreadyInProgress());
        return;
    }

    std::filesystem::path fullPath;
    int listenPort = 8081;
    char clientIp[INET_ADDRSTRLEN]{};
    char mode = 'S';

    {
        std::shared_lock<std::shared_mutex> lock(session.sessionMutex);
        if (!isLoggedIn(session)) {
            sendAll(session.clientFd, ftpNotLoggedIn());
            return;
        }

        std::filesystem::path filename = std::filesystem::path(args[1]).filename();
        fullPath = session.homeDir / session.currentDir / filename;

        listenPort = (session.dataPort > 0) ? session.dataPort : 8081;
        mode = session.transferMode;
        inet_ntop(AF_INET, &session.clientAddress.sin_addr, clientIp, sizeof(clientIp));
    }

    if (!sendSessionReply(session, ftpOpeningDataConnection("STOR"))) {
        return;
    }

    std::thread transferThread(
        rdtReceiveFile,
        fullPath.string(),
        listenPort, clientIp,
        false, &session.isTransferring,
        &session.abortRequested, mode,session.clientFd);
    transferThread.detach();
}

void handleAppe(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (session.isTransferring.load()) {
        sendAll(session.clientFd, ftpTransferAlreadyInProgress());
        return;
    }

    std::filesystem::path fullPath;
    int listenPort = 8081;
    char clientIp[INET_ADDRSTRLEN]{};
    char mode = 'S';

    {
        std::shared_lock<std::shared_mutex> lock(session.sessionMutex);
        if (!isLoggedIn(session)) {
            sendAll(session.clientFd, ftpNotLoggedIn());
            return;
        }

        std::filesystem::path filename = std::filesystem::path(args[1]).filename();
        fullPath = std::filesystem::path("Repository/server_data/Appendables") / filename;

        listenPort = (session.dataPort > 0) ? session.dataPort : 8081;
        mode = session.transferMode;
        inet_ntop(AF_INET, &session.clientAddress.sin_addr, clientIp, sizeof(clientIp));
    }

    if (!sendSessionReply(session, ftpOpeningDataConnection("APPE"))) {
        return;
    }

    std::thread transferThread(
        rdtReceiveFile,
        fullPath.string(),
        listenPort, clientIp, true,
        &session.isTransferring,
        &session.abortRequested, mode,session.clientFd);
    transferThread.detach();
}

void handleAbort(const std::vector<string>& args, ServerSession& session) {
    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    if (!session.isTransferring.load()) {
        sendAll(session.clientFd, "225 No transfer in progress.\r\n");
        return;
    }

    session.abortRequested.store(true);
    sendAll(session.clientFd, ftpTransferAborted());
}

void handleStou(const std::vector<string>& args, ServerSession& session) {
    if (args.size() < 1 || args.size() > 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (session.isTransferring.load()) {
        sendAll(session.clientFd, ftpTransferAlreadyInProgress());
        return;
    }

    std::filesystem::path fullPath;
    int listenPort = 8081;
    char clientIp[INET_ADDRSTRLEN]{};
    char mode = 'S';
    string uniqueFilename;

    {
        std::shared_lock<std::shared_mutex> lock(session.sessionMutex);
        if (!isLoggedIn(session)) {
            sendAll(session.clientFd, ftpNotLoggedIn());
            return;
        }

        string baseName = (args.size() == 2) ? std::filesystem::path(args[1]).stem().string() : "upload";
        string ext = (args.size() == 2) ? std::filesystem::path(args[1]).extension().string() : ".tmp";
        if (ext.empty()) ext = ".tmp";

        uniqueFilename = baseName + ext;
        std::filesystem::path candidatePath = session.homeDir / session.currentDir / uniqueFilename;
        int counter = 1;

        while (std::filesystem::exists(candidatePath)) {
            uniqueFilename = baseName + "_" + std::to_string(counter) + ext;
            candidatePath = session.homeDir / session.currentDir / uniqueFilename;
            counter++;
        }

        fullPath = candidatePath;
        listenPort = (session.dataPort > 0) ? session.dataPort : 8081;
        mode = session.transferMode;
        inet_ntop(AF_INET, &session.clientAddress.sin_addr, clientIp, sizeof(clientIp));
    }

    string reply = "150 FILE: " + uniqueFilename + "\r\n";
    if (!sendSessionReply(session, reply)) {
        return;
    }

    std::thread transferThread(
        rdtReceiveFile,
        fullPath.string(),
        listenPort, clientIp, false,
        &session.isTransferring,
        &session.abortRequested, mode, session.clientFd);
    transferThread.detach();
}



