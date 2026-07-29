#include <cstddef>
#include <iostream>
#include <cstring>     
#include <unistd.h>    
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <string>
#include <vector>

#include <thread>
#include <mutex>

#include "../ResponseRelated/FileManipulation/Upload.h"
#include "../User/User.h"
#include "../ResponseRelated/ResponseHandling/ResponseHandling.h"
#include "../ResponseRelated/Authentication/Username.h"
#include "../ResponseRelated/Authentication/Password.h"
#include "../ResponseRelated/FileManipulation/Retrieve.h"
#include "../ResponseRelated/FileManipulation/fileUtilities.h"
#include "../ResponseRelated/Integrity/Hash.h"


std::vector<User*> users_list = {new User("Son","1234"), new User("Kiet","1234")};

std::mutex session;
std::mutex cli_add;

void handleNewClient(int client_fd,sockaddr_in client_addr,socklen_t client_len){
    sendResponse(client_fd, "125 Connection initiated\n");

    char buffer[1024];
    size_t buffer_capacity = sizeof(buffer) - 1;

    User* user = nullptr;
    bool authenticated = false;

    while (true) {
        memset(buffer, 0, buffer_capacity + 1);
        std::pair<string, string> response = returnResponse(client_fd, buffer, buffer_capacity);

        if (response.first == "EOF") {
            std::cout << "Client disconnected.\n";
            close(client_fd);
            break;
        }

        if (response.first == "USER") {
            user = verifyUserName(users_list, response.second);
            if (user != nullptr) {
                sendResponse(client_fd, "331 Username found. Please enter password (PASSWORD <password>)\n");
            } else {
                sendResponse(client_fd, "530 Invalid username\n");
            }
        }
        else if (response.first == "PASSWORD") {
            if (user != nullptr && verifyPassword(user, response.second)) {
                authenticated = true;
                sendResponse(client_fd, "230 Logged in successfully\n");
            } else {
                sendResponse(client_fd, "530 Incorrect password. Please try again.\n");
            }
        }
        else if (!authenticated) {
            sendResponse(client_fd, "530 Not logged in. Please send USER <username> first.\n");
        }
        else if (response.first == "RETRIEVE") {
            retrieve(response.second, client_addr, client_len);
        }
        else if (response.first == "UPLOAD") {
            sendResponse(client_fd, "127.0.0.1:8080");
            string server_path = "User_uploaded_files/" + getBaseName(response.second);
            upload(server_path);
        }
        else if (response.first == "HASH"){
            string hashed_file = calculateFileSHA256(response.second);
            if (!hashed_file.empty()){
                sendResponse(client_fd, "200 " + hashed_file + "\n");
            } else {
                sendResponse(client_fd, "550 File not found\n");
            }
        }
        else {
            sendResponse(client_fd, "500 Unknown command\n");
        }
    }
}

int main(){

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
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

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        std::cerr << "invalid ip or invalid family address\n";
        close(server_fd);
        return 1;
    }

    if (::bind(server_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "Bind failed\n";
        close(server_fd);
        return 1;
    }

    listen(server_fd, 5);

    while(true){

        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd == -1) {
            std::cerr << "Accept failed\n";
            continue;
        }
        std::thread client_session(handleNewClient,client_fd,client_addr,client_len);
        client_session.detach();
    }
    return 0;
}

