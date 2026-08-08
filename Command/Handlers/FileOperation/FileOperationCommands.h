#pragma once

#include "Architecture/Session/ServerSession.h"

#include <string>
#include <vector>

using std::string;

void handleDele(const std::vector<string>& args, ServerSession& session);
void handleRnfr(const std::vector<string>& args, ServerSession& session);
void handleRnto(const std::vector<string>& args, ServerSession& session);
