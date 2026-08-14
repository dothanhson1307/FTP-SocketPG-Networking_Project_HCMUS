#include "TransferCommands.h"

#include "Architecture/RdtUdp/RDT.h"
#include "Command/Integrity/Hash.h"
#include "SERVER/Server.h"
#include "Helper/FtpReply.h"
#include "Helper/PathHelper.h"
#include "Helper/SocketIO.h"

#include <thread>
#include <arpa/inet.h>
#include <filesystem>
#include <mutex>
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

// Mark the transfer as active before creating its thread.  This prevents a
// second transfer (or ABOR) from seeing a short period where no transfer is
// reported even though the server has already accepted one.
static bool beginTransfer(ServerSession& session) {
    bool expected = false;
    if (!session.isTransferring.compare_exchange_strong(expected, true)) {
        return false;
    }

    session.abortRequested.store(false);
    return true;
}

static void cancelTransferStart(ServerSession& session) {
    session.abortRequested.store(false);
    session.isTransferring.store(false);
}

void handleRetr(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    std::filesystem::path fullPath;
    string requestedFilename;
    sockaddr_in targetUdpAddr{};
    char mode = 'S';

    {
        std::shared_lock<std::shared_mutex> lock(session.sessionMutex);
        if (!isLoggedIn(session)) {
            sendAll(session.clientFd, ftpNotLoggedIn());
            return;
        }

        std::filesystem::path filename = std::filesystem::path(args[1]).filename();
        requestedFilename = filename.string();
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

    const string sourceHash = calculateFileSHA256(fullPath.string());
    if (sourceHash.empty()) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    if (!beginTransfer(session)) {
        sendAll(session.clientFd, ftpTransferAlreadyInProgress());
        return;
    }

    if (!sendSessionReply(session, ftpOpeningDataConnection("RETR"))) {
        cancelTransferStart(session);
        return;
    }
    std::thread transferThread(
        rdtSendFile,
        fullPath.string(),
        targetUdpAddr,
        sizeof(targetUdpAddr),
        &session.isTransferring,
        &session.abortRequested,
        mode,
        session.clientFd,
        [&session, requestedFilename, sourceHash](bool completed) {
            if (!completed) {
                return;
            }

            std::unique_lock<std::shared_mutex> lock(session.sessionMutex);
            session.retrSourceHashes[requestedFilename] = sourceHash;
        }
    );
    transferThread.detach();
}

void handleStor(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    std::filesystem::path fullPath;
    string storedFilename;
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
        storedFilename = filename.string();

        listenPort = (session.dataPort > 0) ? session.dataPort : 8081;
        mode = session.transferMode;
        inet_ntop(AF_INET, &session.clientAddress.sin_addr, clientIp, sizeof(clientIp));
    }

    if (!resolvePathInsideHome(session, args[1], fullPath)) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    if (!beginTransfer(session)) {
        sendAll(session.clientFd, ftpTransferAlreadyInProgress());
        return;
    }

    if (!sendSessionReply(session, ftpOpeningDataConnection("STOR"))) {
        cancelTransferStart(session);
        return;
    }

    std::thread transferThread(
        rdtReceiveFile,
        fullPath.string(),
        listenPort, clientIp,
        false, &session.isTransferring,
        &session.abortRequested, mode, session.clientFd,
        [&session, storedFilename](bool completed) {
            if (!completed) {
                return;
            }

            // HASH <filename> always verifies the most recent successful
            // transfer for that filename.  A completed STOR replaces any
            // earlier RETR verification context with the server-side file.
            std::unique_lock<std::shared_mutex> lock(session.sessionMutex);
            session.retrSourceHashes.erase(storedFilename);
        });
    transferThread.detach();
}

void handleAppe(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
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

    if (!beginTransfer(session)) {
        sendAll(session.clientFd, ftpTransferAlreadyInProgress());
        return;
    }

    if (!sendSessionReply(session, ftpOpeningDataConnection("APPE"))) {
        cancelTransferStart(session);
        return;
    }

    std::thread transferThread(
        rdtReceiveFile,
        fullPath.string(),
        listenPort, clientIp, true,
        &session.isTransferring,
        &session.abortRequested, mode, session.clientFd,
        TransferCompletionCallback{});
    transferThread.detach();
}

void handleAbort(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 1) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    if (!session.isTransferring.load()) {
        sendAll(session.clientFd, "225 No transfer in progress.\r\n");
        return;
    }

    session.abortRequested.store(true);
    // The RDT worker sends the only final 426 reply after it has actually
    // closed the UDP transfer.  Sending one here as well caused duplicate
    // replies on the control connection.
}

void handleStou(const std::vector<string>& args, ServerSession& session) {
    if (args.size() < 1 || args.size() > 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    std::filesystem::path fullPath;
    int listenPort = 8081;
    char clientIp[INET_ADDRSTRLEN]{};
    char mode = 'S';
    string baseName = (args.size() == 2) ? std::filesystem::path(args[1]).stem().string() : "upload";
    string ext = (args.size() == 2) ? std::filesystem::path(args[1]).extension().string() : ".tmp";
    if (ext.empty()) {
        ext = ".tmp";
    }

    string uniqueFilename = baseName + ext;
    int counter = 1;
    while (true) {
        if (!resolvePathInsideHome(session, uniqueFilename, fullPath)) {
            sendAll(session.clientFd, ftpFileUnavailable());
            return;
        }

        if (!std::filesystem::exists(fullPath)) {
            break;
        }

        uniqueFilename = baseName + "_" + std::to_string(counter) + ext;
        counter++;
    }

    {
        std::shared_lock<std::shared_mutex> lock(session.sessionMutex);
        listenPort = (session.dataPort > 0) ? session.dataPort : 8081;
        mode = session.transferMode;
        inet_ntop(AF_INET, &session.clientAddress.sin_addr, clientIp, sizeof(clientIp));
    }

    if (!beginTransfer(session)) {
        sendAll(session.clientFd, ftpTransferAlreadyInProgress());
        return;
    }

    string reply = "150 FILE: " + uniqueFilename + "\r\n";
    if (!sendSessionReply(session, reply)) {
        cancelTransferStart(session);
        return;
    }

    std::thread transferThread(
        rdtReceiveFile,
        fullPath.string(),
        listenPort, clientIp, false,
        &session.isTransferring,
        &session.abortRequested, mode, session.clientFd,
        TransferCompletionCallback{});
    transferThread.detach();
}
