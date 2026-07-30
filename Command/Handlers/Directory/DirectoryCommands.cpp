#include "DirectoryCommands.h"

#include "../../../Helper/FtpReply.h"

#include <filesystem>

using std::string;

string handlePwd(const std::vector<string>& args, ClientData& session) {
    if (args.size() != 1) {
        return ftpInvalidArguments();
    }

    if (!session.loggedIn) {
        return ftpNotLoggedIn();
    }

    const string path = session.homeDir.string() + "\\" + session.currentDir.string();
    return ftpCurrentDirectory(path);
}

string handleMkd(const std::vector<string>& args, ClientData& session) {
    if (args.size() != 2) {
        return ftpInvalidArguments();
    }

    if (!session.loggedIn) {
        return ftpNotLoggedIn();
    }

    const std::filesystem::path directory = session.homeDir / session.currentDir;
    if (std::filesystem::create_directory(directory / args[1])) {
        return ftpDirectoryCreated();
    }

    return ftpCannotCreateDirectory();
}

string handleRmd(const std::vector<string>& args, ClientData& session) {
    if (args.size() != 2) {
        return ftpInvalidArguments();
    }

    if (!session.loggedIn) {
        return ftpNotLoggedIn();
    }

    const std::filesystem::path targetDirectory = session.homeDir / session.currentDir / args[1];
    std::error_code error;
    if (!std::filesystem::is_directory(targetDirectory, error) || error) {
        return ftpDirectoryDoesNotExist();
    }

    if (!std::filesystem::remove(targetDirectory, error) || error) {
        return ftpCannotDeleteDirectory();
    }

    return ftpDirectoryDeleted();
}
