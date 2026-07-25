#pragma once
#include <string>
#include "../../User/User.h"
#include "../ResponseHandling/ResponseHandling.h"

using std::string;

void sendPasswordAuthentication(int client_fd);
bool verifyPassword(User* user, const string& password);
