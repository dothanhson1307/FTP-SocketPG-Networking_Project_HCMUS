#include "DirectoryCommands.h"

#include "Helper/FtpReply.h"
#include "Helper/PathHelper.h"
#include "Helper/SocketIO.h"

#include <filesystem>

using std::string;

static bool isLoggedIn(const ServerSession& session) {
    return session.loggedIn && !session.homeDir.empty();
}

void handlePwd(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 1) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    sendAll(session.clientFd, ftpCurrentDirectory(session.currentDir.generic_string()));
}

void handleCwd(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    std::filesystem::path resolvedPath;
    if (!resolvePathInsideHome(session, args[1], resolvedPath)) {
        sendAll(session.clientFd, ftpDirectoryDoesNotExist());
        return;
    }

    std::error_code error;
    if (!std::filesystem::is_directory(resolvedPath, error) || error) {
        sendAll(session.clientFd, ftpDirectoryDoesNotExist());
        return;
    }

    const auto relative = resolvedPath.lexically_relative(session.homeDir);
    if (relative.empty()) {
        sendAll(session.clientFd, ftpDirectoryDoesNotExist());
        return;
    }

    session.currentDir = relative;
    sendAll(session.clientFd, ftpCommandSuccessful("Directory successfully changed."));
}

void handleMkd(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    std::filesystem::path newDir;
    if (!resolvePathInsideHome(session, args[1], newDir)) {
        sendAll(session.clientFd, ftpCannotCreateDirectory());
        return;
    }

    std::error_code error;
    if (!std::filesystem::create_directory(newDir, error) || error) {
        sendAll(session.clientFd, ftpCannotCreateDirectory());
        return;
    }

    sendAll(session.clientFd, ftpDirectoryCreated());
}

void handleRmd(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    std::filesystem::path targetDir;
    if (!resolvePathInsideHome(session, args[1], targetDir)) {
        sendAll(session.clientFd, ftpDirectoryDoesNotExist());
        return;
    }

    std::error_code error;
    if (!std::filesystem::is_directory(targetDir, error) || error) {
        sendAll(session.clientFd, ftpDirectoryDoesNotExist());
        return;
    }

    if (std::filesystem::remove(targetDir, error) && !error) {
        sendAll(session.clientFd, ftpDirectoryDeleted());
        return;
    }

    sendAll(session.clientFd, ftpCannotDeleteDirectory());
}

void handleCdup(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 1) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    session.currentDir = session.currentDir.parent_path();
    if(session.currentDir == "") session.currentDir = ".";

    sendAll(session.clientFd, ftpCommandSuccessful("Directory successfully changed."));
}
