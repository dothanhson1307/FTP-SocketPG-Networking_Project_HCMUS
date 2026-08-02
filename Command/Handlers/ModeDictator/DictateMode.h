#pragma once

#include "Architecture/Session/ServerSession.h"
#include <string>
#include <vector>

/**
 * Handles the PORT command (Active Mode).
 * Syntax: PORT h1,h2,h3,h4,p1,p2
 * Configures client IP and data port for server-initiated data transfers.
 */
std::string handleActiveMode(const std::vector<std::string>& args, ServerSession& session);

/**
 * Handles the PASV command (Passive Mode).
 * Syntax: PASV
 * Tells the server to listen on a data port and returns the IP and port for client connections.
 * Returns: 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)
 */
std::string handlePassiveMode(const std::vector<std::string>& args, ServerSession& session);

