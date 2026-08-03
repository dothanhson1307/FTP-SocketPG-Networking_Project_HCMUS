#pragma once

#include "Architecture/Session/ServerSession.h"

#include <string>
#include <vector>

using std::string;

string handleRetr(const std::vector<string>& args, ServerSession& session);
string handleStor(const std::vector<string>& args, ServerSession& session);
string handleAppe(const std::vector<string>& args, ServerSession& session);

string handleAbort(const std::vector<string>& args, ServerSession& session);

string handleHash(const std::vector<string>& args, ServerSession& session);
