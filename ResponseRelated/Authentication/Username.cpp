#include "Username.h"
#include <algorithm>
#include <cctype>

void sendUsernameAuthentication(int client_fd){
    string response = "USER <username>\n";
    sendResponse(client_fd, response);
}

User* verifyUserName(const std::vector<User*>& vec, const string& username){
    string input_lower = username;
    std::transform(input_lower.begin(), input_lower.end(), input_lower.begin(), ::tolower);

    for (User* user : vec) {
        string stored_lower = user->getUsername();
        std::transform(stored_lower.begin(), stored_lower.end(), stored_lower.begin(), ::tolower);

        if (stored_lower == input_lower) {
            return user;
        }
    }
    return nullptr;
}
