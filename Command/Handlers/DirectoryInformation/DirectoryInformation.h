#pragma once

#include "Architecture/Session/ServerSession.h"

#include <string>
#include <vector>

using std::string;

void handleList(const std::vector<string>& args, ServerSession& session);
void handleNlst(const std::vector<string>& args, ServerSession& session);
void handleStat(const std::vector<string>& args, ServerSession& session);
void handleSize(const std::vector<string>& args, ServerSession& session);
void handleMdtm(const std::vector<string>& args, ServerSession& session);