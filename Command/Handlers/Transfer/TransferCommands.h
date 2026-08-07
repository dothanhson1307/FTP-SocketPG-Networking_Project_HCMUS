#pragma once

#include "Architecture/Session/ServerSession.h"

#include <string>
#include <vector>

using std::string;

void handleRetr(const std::vector<string>& args, ServerSession& session);
void handleStor(const std::vector<string>& args, ServerSession& session);
void handleAppe(const std::vector<string>& args, ServerSession& session);
void handleStou(const std::vector<string>& args, ServerSession& session);

void handleAbort(const std::vector<string>& args, ServerSession& session);


