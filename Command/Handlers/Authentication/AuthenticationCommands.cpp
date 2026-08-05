#include "AuthenticationCommands.h"
#include "Helper/FtpReply.h"
#include "Helper/SocketIO.h"
#include <filesystem>
#include <algorithm>

using std::string;

static bool isValidUser(const string& username) {
    string lower = username;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return (lower == "son" || lower == "kiet");
}

void handleUser(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    const string& username = args[1];
    if (isValidUser(username)) {
        session.username = username;
        session.usernameAccepted = true;
        session.loggedIn = false;
        sendAll(session.clientFd, ftpUsernameAccepted());
        return;
    }

    session.username.clear();
    session.usernameAccepted = false;
    session.loggedIn = false;
    sendAll(session.clientFd, ftpNotLoggedIn());
}

void handlePass(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!session.usernameAccepted || session.loggedIn) {
        sendAll(session.clientFd, ftpBadSequence());
        return;
    }

    const string& password = args[1];
    if (password != "1234" && password != "son123" && password != "kiet123") {
        session.loggedIn = false;
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
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
    sendAll(session.clientFd, ftpLoginSuccessful());
}

void handleQuit(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 1) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    {
        std::unique_lock<std::shared_mutex> lock(session.sessionMutex);
        session.abortRequested.store(true);
        session.quitRequested = true;
    }

    sendAll(session.clientFd, ftpGoodbye());
}

