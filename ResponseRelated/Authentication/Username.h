#pragma once
#include <string>
#include <vector>
#include "../../User/User.h"
#include "../ResponseHandling/ResponseHandling.h"

using std::string;

void sendUsernameAuthentication(int client_fd);
User* verifyUserName(const std::vector<User*>& vec, const string& username);
