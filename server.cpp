#include "User/User.h"
#include <iostream>
#include <cstring>      // For memset / strlen
#include <unistd.h>     // For close() and ssize_t
#include <sys/socket.h> // For socket(), bind(), listen(), accept(), send(), recv()
#include <netinet/in.h> // For sockaddr_in, htons(), INADDR_ANY
#include <arpa/inet.h>  // For inet_ntoa()
#include <vector>
#include <string>

void sendResponse(int client_fd, const std::string& msg){
    send(client_fd, msg.c_str(), msg.length(), 0);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "[Server] Failed to create socket!\n";
        return -1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "[Server] Bind failed! Is port 8080 already open?\n";
        close(server_fd);
        return -1;
    }

    listen(server_fd, 5);

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (client_fd < 0) {
        std::cerr << "[Server] Failed to accept client connection!\n";
        close(server_fd);
        return -1;
    }

    std::cout << "[Server] Client connected from IP: " << inet_ntoa(client_addr.sin_addr) << "\n";

    char buffer[1024]; 
    size_t buffer_capacity = sizeof(buffer) - 1; 

    std::string current_username = "";
    bool logged_in = false;

    std::vector<User> valid_users = {
        User("admin", "1234"),
        User("user", "password")
    };

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_received = recv(client_fd, buffer, buffer_capacity, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string request(buffer);
            request.erase(request.find_last_not_of("\r\n") + 1);
            std::cout << "[Server] Received: \"" << request << "\"\n";

            std::string command;
            std::string argument;
            size_t space_pos = request.find(' ');
            if (space_pos != std::string::npos) {
                command = request.substr(0, space_pos);
                argument = request.substr(space_pos + 1);
            } else {
                command = request;
            }

        } else if (bytes_received == 0) {
            std::cout << "[Server] Client closed the connection.\n";
            break;
        } else {
            std::cerr << "[Server] Fatal error reading from socket!\n";
            break;
        }
    }
    close(client_fd); 
    close(server_fd); 
    std::cout << "[Server] Sockets closed. Goodbye.\n";
    return 0;
}