#pragma once

#include "Architecture/Session/ServerSession.h"
#include <string>
#include <vector>

/**
 * Handles the PORT command (Active Mode).
 * Syntax: PORT h1,h2,h3,h4,p1,p2
 * Configures client IP and data port for server-initiated data transfers.
 */
void handleActiveMode(const std::vector<std::string>& args, ServerSession& session);
void handlePassiveMode(const std::vector<std::string>& args, ServerSession& session);

