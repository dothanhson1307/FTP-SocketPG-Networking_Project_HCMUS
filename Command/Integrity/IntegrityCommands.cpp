#include "IntegrityCommands.h"

#include "Command/Integrity/Hash.h"
#include "Helper/FtpReply.h"
#include "Helper/SocketIO.h"

#include <filesystem>
#include <shared_mutex>
#include <vector>

using std::string;

static bool isLoggedIn(const ServerSession& session) {
    return session.loggedIn && !session.homeDir.empty();
}

void handleHash(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    std::filesystem::path fullPath;
    string retrSourceHash;
    {
        std::shared_lock<std::shared_mutex> lock(session.sessionMutex);
        if (!isLoggedIn(session)) {
            sendAll(session.clientFd, ftpNotLoggedIn());
            return;
        }

        std::filesystem::path filename = std::filesystem::path(args[1]).filename();
        const auto cachedHash = session.retrSourceHashes.find(filename.string());
        if (cachedHash != session.retrSourceHashes.end()) {
            retrSourceHash = cachedHash->second;
        }

        fullPath = session.homeDir / session.currentDir / filename;
    }

    // A successful RETR stores the source hash because that source lives in
    // Downloadable_files, outside the normal user home directory.
    if (!retrSourceHash.empty()) {
        sendAll(session.clientFd, "213 RETR-PRE " + retrSourceHash + "\r\n");
        return;
    }

    string hashValue = calculateFileSHA256(fullPath.string());
    if (hashValue.empty()) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    sendAll(session.clientFd, ftpSha256(hashValue));
}
