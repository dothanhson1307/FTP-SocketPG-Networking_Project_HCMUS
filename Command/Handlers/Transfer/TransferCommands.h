#pragma once

#include "../../Session/ClientData.h"

#include <string>
#include <vector>

using std::string;

// could be declared with constexpr to reduce time complexity
const int kDataPort = 8081;


string handleRetr(const std::vector<string>& args, ClientData& session);
string handleStor(const std::vector<string>& args, ClientData& session);
string handleHash(const std::vector<string>& args, ClientData& session);
