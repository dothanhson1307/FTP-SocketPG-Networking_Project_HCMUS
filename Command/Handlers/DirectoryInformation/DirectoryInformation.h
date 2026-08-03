#pragma once

#include "Architecture/Session/ServerSession.h"

#include <string>
#include <vector>

using std::string;

string handleList(const std::vector<string>& args, ServerSession& session);
string handleNlst(const std::vector<string>& args, ServerSession& session);
string handleStat(const std::vector<string>& args, ServerSession& session);
string handleSize(const std::vector<string>& args, ServerSession& session);
string handleMdtm(const std::vector<string>& args, ServerSession& session);
