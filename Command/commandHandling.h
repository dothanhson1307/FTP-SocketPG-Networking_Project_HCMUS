#pragma once

#include "../User/User.h"

#include <functional>
#include <string>
#include <vector>

using std::string;


// manage number of people beign listened, if out of range --> give more listen slots. 
struct SessionState {
    string username;   
    bool usernameAccepted = false;
    bool loggedIn = false;
    bool quitRequested = false;
};

// using CommandArguments = std::vector<std::string>;
using CommandHandler = std::function<std::string(const std::vector<string>&, SessionState&)>;

std::vector<std::vector<string>> read(string&);
