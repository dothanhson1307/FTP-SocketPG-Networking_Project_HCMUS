#pragma once

#include "../../Session/ClientData.h"

#include <string>
#include <vector>

using std::string;

string handleUser(const std::vector<string>& args, ClientData& session);
string handlePass(const std::vector<string>& args, ClientData& session);
string handleQuit(const std::vector<string>& args, ClientData& session);
