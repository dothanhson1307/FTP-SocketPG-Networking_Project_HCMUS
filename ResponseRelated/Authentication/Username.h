#pragma once
#include <string>
#include <vector>
#include "../../User/user.h"
#include "../ResponseHandling/ResponseHandling.h"

using std::string;

void sendUsernameAuthentication(int client_fd);
User* verifyUserName(const std::vector<User*>& vec, const string& username);
