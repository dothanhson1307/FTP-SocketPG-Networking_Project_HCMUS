#include "FileOperationCommands.h"

#include "Helper/FtpReply.h"
#include "Helper/PathHelper.h"
#include "Helper/SocketIO.h"

#include <filesystem>

static bool isLoggedIn(const ServerSession& session) {
    return session.loggedIn && !session.homeDir.empty();
}

void handleDele(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    std::filesystem::path target;
    if (!resolvePathInsideHome(session, args[1], target)) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    // delete regular file only. Do not delete folder
    std::error_code error;
    if (!std::filesystem::is_regular_file(target, error) || error) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    if (!std::filesystem::remove(target, error) || error) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    sendAll(session.clientFd, ftpFileDeleted());
}

void handleRnfr(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    std::filesystem::path source;
    if (!resolvePathInsideHome(session, args[1], source)) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    std::error_code error;
    if (!std::filesystem::exists(source, error) || error) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    session.renameSourcePath = source;
    session.renamePending = true;
    
    sendAll(session.clientFd, ftpRenameReady());
}

void handleRnto(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    if (!session.renamePending) {
        sendAll(session.clientFd, ftpBadSequence());
        return;
    }

    const std::filesystem::path source = session.renameSourcePath;
    session.renameSourcePath.clear();
    session.renamePending = false;

    std::filesystem::path target;
    if (!resolvePathInsideHome(session, args[1], target)) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    std::error_code error;
    if (std::filesystem::exists(target, error) || error) {
        sendAll(session.clientFd, ftpFileAlreadyExists());
        return;
    }

    std::filesystem::rename(source, target, error);
    if (error) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    sendAll(session.clientFd, ftpRenameSuccessful());
}
