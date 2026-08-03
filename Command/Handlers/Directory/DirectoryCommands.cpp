#include "DirectoryCommands.h"

#include "Helper/FtpReply.h"
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

    std::filesystem::path target = args[1];
    std::filesystem::path newCurrent = session.currentDir;

    if (target.is_absolute()) {
        newCurrent = target.relative_path();
    } else {
        newCurrent /= target;
    }

    newCurrent = newCurrent.lexically_normal();

    std::filesystem::path resolvedPath = session.homeDir / newCurrent;

    std::error_code error;
    if (!std::filesystem::is_directory(resolvedPath, error) || error) {
        sendAll(session.clientFd, ftpDirectoryDoesNotExist());
        return;
    }

    auto relative = std::filesystem::relative(resolvedPath, session.homeDir, error);
    if (error || relative.empty() || relative.string().find("..") == 0) {
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

    std::filesystem::path newDir = session.homeDir / session.currentDir / args[1];

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

    std::filesystem::path targetDir = session.homeDir / session.currentDir / args[1];

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
