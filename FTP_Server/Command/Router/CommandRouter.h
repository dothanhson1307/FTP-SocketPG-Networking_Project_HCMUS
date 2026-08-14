#pragma once

#include "Architecture/Session/ServerSession.h"

#include <functional>
#include <string>
#include <vector>

using std::string;

using CommandArguments = std::vector<string>;
using CommandHandler = std::function<void(const CommandArguments&, ServerSession&)>;

std::vector<CommandArguments> extractCommandTokens(string& pendingData);
void executeCommand(const CommandArguments& args, ServerSession& session);
