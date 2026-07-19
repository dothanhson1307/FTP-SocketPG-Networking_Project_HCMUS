#include <cstddef>
#include <iostream>
#include <cstring>     
#include <unistd.h>    
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <string>
#include <vector>
#include "User/User.h"

using namespace std;

void sendResponse(int client_fd, const string& response){
    ssize_t signal = send(client_fd, response.c_str(), response.length(), 0);
    if(signal > 0) std::cout<<"Sent successfully\n";
    else std::cerr<<"Failed to send response\n";
}

void sendUsernameAuthentication(int client_fd){
    string response = "USER <username>\n";
    sendResponse(client_fd,response);
}

void sendPasswordAuthentication(int client_fd){
    string response = "PASSWORD <password>\n";
    sendResponse(client_fd,response);
}

User* verifyUserName(const std::vector<User*>& vec, const string username){
    for(User* user : vec){
        if(user->getUsername() == username){
            return user;
        }
    }
    return nullptr;
}

std::pair<string,string> returnResponse(int client_fd, char* buffer, size_t buffer_capacity){

    string command;
    string argument;

    ssize_t bytes_received = recv(client_fd, buffer, buffer_capacity, 0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        string request(buffer);
        request.erase(request.find_last_not_of("\r\n") + 1);
        std::cout << "[Server] Received: \"" << request << "\"\n";
        size_t space_pos = request.find(' ');

        if (space_pos != string::npos) {
            command = request.substr(0, space_pos);
            argument = request.substr(space_pos + 1);
        } else {
            command = request;
        }
    } else {
        command = "EOF";
    }
    return std::pair<string,string>(command,argument);
}


int main(){
    std::vector<User*> users_list = {new User("Son","1234"),new User("Kiet","1234")};

    int server_fd = socket(AF_INET,SOCK_STREAM,0);
    if(server_fd==-1){
        std::cerr << "kernel can't reserve space for socket\n";
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt(SO_REUSEADDR) failed\n";
        close(server_fd);
        return 1;
    }

    sockaddr_in server_addr{};

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);

    if(inet_pton(AF_INET,"127.0.0.1",&server_addr.sin_addr)<=0){
        std::cerr << "invalid ip or invalid family address\n";
        close(server_fd);
        return 1;
    }

    if (::bind(server_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "Bind failed\n";
        close(server_fd);
        return 1;
    }

    listen(server_fd,5);

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(server_fd,reinterpret_cast<sockaddr*>(&client_addr),&client_len);

    char buffer[1024];
    size_t buffer_capacity = sizeof(buffer) - 1;

    sendUsernameAuthentication(client_fd);

    while(true){
        memset(buffer,0,buffer_capacity + 1);
        std::pair<string,string> response = returnResponse(client_fd,buffer,buffer_capacity);

        if(response.first == "EOF"){
            std::cout << "Client disconnected.\n";
            close(client_fd);
            break;
        }

        if(response.first == "USER"){
            User* user = verifyUserName(users_list, response.second);
            if (user != nullptr) {
                sendPasswordAuthentication(client_fd);
            } else {
                sendResponse(client_fd, "530 Invalid username\n");
            }
        }
        
    }
    return 0;
}