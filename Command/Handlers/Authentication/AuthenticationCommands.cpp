#include "AuthenticationCommands.h"
#include "Helper/FtpReply.h"
#include <filesystem>
#include <algorithm>

using std::string;

static bool isValidUser(const string& username) {
    string lower = username;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return (lower == "son" || lower == "kiet");
}

string handleUser(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        return ftpInvalidArguments();
    }

    const string& username = args[1];
    if (isValidUser(username)) {
        session.username = username;
        session.usernameAccepted = true;
        session.loggedIn = false;
        return ftpUsernameAccepted();
    }

    session.username.clear();
    session.usernameAccepted = false;
    session.loggedIn = false;
    return ftpNotLoggedIn();
}

string handlePass(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        return ftpInvalidArguments();
    }

    if (!session.usernameAccepted || session.loggedIn) {
        return ftpBadSequence();
    }

    const string& password = args[1];
    if (password != "1234" && password != "son123" && password != "kiet123") {
        session.loggedIn = false;
        return ftpNotLoggedIn();
    }

    std::filesystem::path userHome = std::filesystem::absolute(
        std::filesystem::path("Repository/user_data") / session.username
    );
    std::error_code error;
    if (!std::filesystem::is_directory(userHome, error) || error) {
        error.clear();
        std::filesystem::create_directories(userHome, error);
    }

    session.loggedIn = true;
    session.homeDir = userHome;
    session.currentDir = ".";
    return ftpLoginSuccessful();
}

string handleQuit(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 1) {
        return ftpInvalidArguments();
    }

    session.quitRequested = true;
    return ftpGoodbye();
}

string handleNoop(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 1) {
        return ftpInvalidArguments();
    }

    return ftpCommandSuccessful("NOOP successful; session remains active.");
}