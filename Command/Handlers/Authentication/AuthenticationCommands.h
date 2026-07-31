#pragma once

#include "Architecture/Session/ServerSession.h"

#include <string>
#include <vector>

using std::string;

string handleUser(const std::vector<string>& args, ServerSession& session);
string handlePass(const std::vector<string>& args, ServerSession& session);
string handleQuit(const std::vector<string>& args, ServerSession& session);
