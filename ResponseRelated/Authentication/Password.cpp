#include "Password.h"

void sendPasswordAuthentication(int client_fd){
    string response = "PASSWORD <password>\n";
    sendResponse(client_fd,response);
}

bool verifyPassword(User* user,const string& password){
    return (user->getPassword() == password);
}
