#pragma once

#include "../../Session/ClientData.h"

#include <string>
#include <vector>

using std::string;

string handlePwd(const std::vector<string>& args, ClientData& session);
string handleMkd(const std::vector<string>& args, ClientData& session);
string handleRmd(const std::vector<string>& args, ClientData& session);
