#include "command.h"

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

string handleUser(const std::vector<string>& args, SessionState& session) {
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

string handlePass(const std::vector<string>& args, SessionState& session) {
    if (args.size() != 2) {
        return "501 Syntax error in parameters or arguments!\r\n";
    }
    
    if(!session.usernameAccepted) {
        return "503 bad sequence of commands!\r\n";
    }
    
    User target(session.username, args[1]);
    
    if(findUser(target)) {
        session.loggedIn = true;
        return "230 Login successfuly!\r\n";
    }
    
    session.loggedIn = false;
    return "530 Not logged in!\r\n";
}

string handleQuit(const std::vector<string>& args, SessionState& session) {
    if (args.size() != 1) {
        return "501 Syntax error in parameters or arguments!\r\n";
    }
    
    session.quitRequested = true;
    
    return "221 Goodbye!\r\n";
}