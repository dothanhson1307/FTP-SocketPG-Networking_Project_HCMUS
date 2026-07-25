#pragma once

#include "User/User.h"

#include <functional>
#include <string>
#include <vector>

using std::string;

struct SessionState {
    std::string username;
    bool usernameAccepted = false;
    bool loggedIn = false;
    bool quitRequested = false;
};

// using CommandArguments = std::vector<std::string>;
using CommandHandler = std::function<std::string(const std::vector<string>&, SessionState&)>;

bool read(const string&, std::vector<std::vector<string>>&);
