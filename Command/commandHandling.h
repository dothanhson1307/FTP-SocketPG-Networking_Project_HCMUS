#pragma once

#include "../User/User.h"

#include <functional>
#include <string>
#include <vector>
#include <filesystem>

using std::string;
namespace fs = std::filesystem;


// manage number of people beign listened, if out of range --> give more listen slots. 
struct ClientData {
    string username;   
    bool usernameAccepted = false;
    bool loggedIn = false;
    bool quitRequested = false;

    fs::path homeDir;
    fs::path currentDir;
};

// using CommandArguments = std::vector<std::string>;
using CommandHandler = std::function<std::string(const std::vector<string>&, ClientData&)>;

std::vector<std::vector<string>> read(string&);
