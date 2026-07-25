#pragma once 

#include "../User/User.h"
#include "commandHandling.h"

using std::string;

bool findUser(User target);

std::string handleUser(const std::vector<string>& args, SessionState& session);
std::string handlePass(const std::vector<string>& args, SessionState& session);
std::string handleQuit(const std::vector<string>& args, SessionState& session);
std::string executeCommand(const std::vector<string>& args, SessionState& session);