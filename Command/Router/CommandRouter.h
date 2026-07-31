#pragma once

#include "Architecture/Session/ServerSession.h"

#include <functional>
#include <string>
#include <vector>

using std::string;

using CommandArguments = std::vector<string>;
using CommandHandler = std::function<string(const CommandArguments&, ServerSession&)>;

std::vector<CommandArguments> extractCommandTokens(string& pendingData);
string executeCommand(const CommandArguments& args, ServerSession& session);
