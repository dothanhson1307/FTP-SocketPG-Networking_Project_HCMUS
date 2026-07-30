#include "AuthenticationCommands.h"

#include "../../../User/User.h"
#include "../../../Helper/FtpReply.h"

#include <filesystem>

using std::string;

static bool findUser(User target) {
    for (User& account : Accounts) {
        if (
            account.getUsername() == target.getUsername()
            && account.getPassword() == target.getPassword()
        ) {
            return true;
        }
    }
    return false;
}


string handleUser(const std::vector<string>& args, ClientData& session) {
    if (args.size() != 2) {
        return ftpInvalidArguments();
    }

    const string& username = args[1];
    for (User& account : Accounts) {
        if (account.getUsername() == username) {
            session.username = username;
            session.usernameAccepted = true;
            session.loggedIn = false;
            return ftpUsernameAccepted();
        }
    }

    session.username.clear();
    session.usernameAccepted = false;
    session.loggedIn = false;
    return ftpNotLoggedIn();
}

string handlePass(const std::vector<string>& args, ClientData& session) {
    if (args.size() != 2) {
        return ftpInvalidArguments();
    }

    if (!session.usernameAccepted || session.loggedIn) {
        return ftpBadSequence();
    }

    User target(session.username, args[1]);
    if (!findUser(target)) {
        session.loggedIn = false;
        return ftpNotLoggedIn();
    }

    std::filesystem::path userHome = std::filesystem::absolute(
        std::filesystem::path("user_data") / session.username
    );
    std::error_code error;
    if (!std::filesystem::is_directory(userHome, error) || error) {
        error.clear();
        if (!std::filesystem::create_directories(userHome, error) && error) {
            return ftpCannotCreateUserDirectory();
        }
    }

    session.loggedIn = true;
    session.homeDir = userHome;
    session.currentDir = ".";
    return ftpLoginSuccessful();
}

string handleQuit(const std::vector<string>& args, ClientData& session) {
    if (args.size() != 1) {
        return ftpInvalidArguments();
    }

    session.quitRequested = true;
    return ftpGoodbye();
}
