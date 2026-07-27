#include "command.h"
#include <filesystem>

namespace fs = std::filesystem;

bool findUser(User target) {
    for (User& account : Accounts) {
        if (
            account.getUsername() == target.getUsername() &&
            account.getPassword() == target.getPassword()
        ) {
            return true;
        }
    }
    return false;
}

string handleUser(const std::vector<string>& args, ClientData& session) {
    if(args.size() != 2) {
        return "501 Syntax error in parameters or arguments!\r\n";
    }
    
    const string& username = args[1];
    for (User& account : Accounts) {
        if (account.getUsername() == username) {
            session.username = username;
            session.usernameAccepted = true;
            session.loggedIn = false;
            
            return "331 Username correct! Need password\r\n";
        }
    }
    
    session.username.clear();
    session.usernameAccepted = false;
    session.loggedIn = false;
    
    return "530 Not logged in!\r\n";
}

string handlePass(const std::vector<string>& args, ClientData& session) {
    if (args.size() != 2) {
        return "501 Syntax error in parameters or arguments!\r\n";
    }
    
    if(!session.usernameAccepted || session.loggedIn) {
        return "503 Bad sequence of commands!\r\n";
    }

    User target(session.username, args[1]);
    
    if(findUser(target)) {
        session.loggedIn = true;
        
        
        fs::path userHome = fs::absolute(fs::path("user_data") / session.username);
        
        std::error_code ec;
        if (!fs::is_directory(userHome, ec) || ec) {
            ec.clear();

            if (!fs::create_directories(userHome, ec) && ec) {
                return "550 Cannot create user directory!\r\n";
            }
        }

        session.homeDir = userHome;
        session.currentDir = ".";
        return "230 Login successfuly!\r\n";
    }
    
    session.loggedIn = false;
    return "530 Not logged in!\r\n";
}

string handleQuit(const std::vector<string>& args, ClientData& session) {
    if (args.size() != 1) {
        return "501 Syntax error in parameters or arguments!\r\n";
    }
    
    session.quitRequested = true;
    
    return "221 Goodbye!\r\n";
}

string handlePwd(const std::vector<string>& args, ClientData& session) {
    if (args.size() != 1) {
        return "501 Syntax error in parameters or arguments!\r\n";
    }    

    if(!session.loggedIn) {
        return "530 Not logged in!\r\n";
    }
    string path_str = session.homeDir.string() + "\\" + session.currentDir.string();
    return "257 \"" + path_str + "\" is the current directory!\r\n";
}

string handleMkd(const std::vector<string>& args, ClientData& session) {
    if (args.size() != 2) {
        return "501 Syntax error in parameters or arguments!\r\n";
    }

    if(!session.loggedIn) {
        return "530 Not logged in!\r\n";
    }

    fs::path dir = session.homeDir / session.currentDir;
    if (fs::create_directory(dir / args[1])) {
        return "257 Create new folder successfuly!\r\n";
    }

    return "550 Cannot create directory!\r\n";
}

string handleRmd(const std::vector<string>& args, ClientData& session) {
    if (args.size() != 2) {
        return "501 Syntax error in parameters or arguments!\r\n";
    }

    if(!session.loggedIn) {
        return "530 Not logged in!\r\n";
    }

    fs::path dir = session.homeDir / session.currentDir;
    fs::path targetDir = dir / args[1];

    std::error_code ec;

    if (!fs::is_directory(targetDir, ec) || ec) {
        return "550 Directory does not exist.\r\n";
    }

    if (!fs::remove(targetDir, ec) || ec) {
        return "550 Cannot delete directory. It may not be empty.\r\n";
    }

    return "250 Directory deleted successfully.\r\n";
}