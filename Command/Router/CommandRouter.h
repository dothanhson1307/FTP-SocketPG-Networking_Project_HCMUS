#pragma once

#include "../Session/ClientData.h"

#include <functional>
#include <string>
#include <vector>

using std::string;

using CommandArguments = std::vector<string>;
using CommandHandler = std::function<string(const CommandArguments&, ClientData&)>;

std::vector<CommandArguments> read(string& pendingData);
string executeCommand(const CommandArguments& args, ClientData& session);
