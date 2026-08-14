#pragma once

#include "Architecture/Session/ServerSession.h"

#include <string>
#include <vector>

using std::string;

void handleUser(const std::vector<string>& args, ServerSession& session);
void handlePass(const std::vector<string>& args, ServerSession& session);
void handleQuit(const std::vector<string>& args, ServerSession& session);
void handleNoop(const std::vector<string>& args, ServerSession& session);
