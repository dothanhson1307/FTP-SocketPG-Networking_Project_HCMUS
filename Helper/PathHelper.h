#pragma once

#include "Architecture/Session/ServerSession.h"

#include <filesystem>
#include <string>

// check if it's absolutely path
// prevent ../ ; combine homeDir + currentDir + input
// check if the final path is in homeDir
bool resolvePathInsideHome(const ServerSession& session, const std::string& input, std::filesystem::path& resolvedPath);
