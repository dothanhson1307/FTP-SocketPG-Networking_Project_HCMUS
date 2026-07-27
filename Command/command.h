#pragma once 

#include "../User/User.h"
#include "commandHandling.h"

using std::string;

bool findUser(User target);

std::string handleUser(const std::vector<string>& args, ClientData& session);
std::string handlePass(const std::vector<string>& args, ClientData& session);
std::string handleQuit(const std::vector<string>& args, ClientData& session);

std::string handlePwd(const std::vector<string>& args, ClientData& session);
std::string handleCwd(const std::vector<string>& args, ClientData& session);
std::string handleMkd(const std::vector<string>& args, ClientData& session);
std::string handleRmd(const std::vector<string>& args, ClientData& session);

std::string executeCommand(const std::vector<string>& args, ClientData& session);