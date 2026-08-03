#pragma once

#include "Architecture/Session/ServerSession.h"

#include <string>
#include <vector>

using std::string;

void handlePwd(const std::vector<string>& args, ServerSession& session);
void handleCwd(const std::vector<string>& args, ServerSession& session);
void handleMkd(const std::vector<string>& args, ServerSession& session);
void handleRmd(const std::vector<string>& args, ServerSession& session);
void handleCdup(const std::vector<string>& args, ServerSession& session);
