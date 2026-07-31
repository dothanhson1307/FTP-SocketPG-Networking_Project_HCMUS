#pragma once

#include "Architecture/Session/ServerSession.h"

#include <string>
#include <vector>

using std::string;

string handlePwd(const std::vector<string>& args, ServerSession& session);
string handleCwd(const std::vector<string>& args, ServerSession& session);
string handleMkd(const std::vector<string>& args, ServerSession& session);
string handleRmd(const std::vector<string>& args, ServerSession& session);
