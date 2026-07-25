#include "Username.h"

void sendUsernameAuthentication(int client_fd){
    string response = "USER <username>\n";
    sendResponse(client_fd,response);
}

User* verifyUserName(const std::vector<User*>& vec, const string& username){
    for(User* user : vec){
        if(user->getUsername() == username){
            return user;
        }
    }
    return nullptr;
}
