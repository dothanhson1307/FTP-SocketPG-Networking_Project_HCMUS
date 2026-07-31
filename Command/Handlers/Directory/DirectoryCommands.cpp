#include "DirectoryCommands.h"

#include "Helper/FtpReply.h"

#include <filesystem>

using std::string;

static bool isLoggedIn(const ServerSession& session) {
    return session.loggedIn && !session.homeDir.empty();
}

string handlePwd(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 1) {
        return ftpInvalidArguments();
    }

    if (!isLoggedIn(session)) {
        return ftpNotLoggedIn();
    }

    return ftpCurrentDirectory(session.currentDir.generic_string());
}

string handleCwd(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        return ftpInvalidArguments();
    }

    if (!isLoggedIn(session)) {
        return ftpNotLoggedIn();
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
        return ftpDirectoryDoesNotExist();
    }

    auto relative = std::filesystem::relative(resolvedPath, session.homeDir, error);
    if (error || relative.empty() || relative.string().find("..") == 0) {
        return ftpDirectoryDoesNotExist();
    }

    session.currentDir = relative;
    return ftpLoginSuccessful();
}

string handleMkd(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        return ftpInvalidArguments();
    }

    if (!isLoggedIn(session)) {
        return ftpNotLoggedIn();
    }

    std::filesystem::path newDir = session.homeDir / session.currentDir / args[1];

    std::error_code error;
    if (!std::filesystem::create_directory(newDir, error) || error) {
        return ftpCannotCreateDirectory();
    }

    return ftpDirectoryCreated();
}

string handleRmd(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        return ftpInvalidArguments();
    }

    if (!isLoggedIn(session)) {
        return ftpNotLoggedIn();
    }

    std::filesystem::path targetDir = session.homeDir / session.currentDir / args[1];

    std::error_code error;
    if (!std::filesystem::is_directory(targetDir, error) || error) {
        return ftpDirectoryDoesNotExist();
    }

    if (std::filesystem::remove(targetDir, error) && !error) {
        return ftpDirectoryDeleted();
    }

    return ftpCannotDeleteDirectory();
}
