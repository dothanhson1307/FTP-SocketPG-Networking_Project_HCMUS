#include "../ResponseRelated/FileManipulation/Retrieve.h"
#include "../ResponseRelated/FileManipulation/Upload.h"
#include "../ResponseRelated/ResponseHandling/ResponseHandling.h"
#include "../ResponseRelated/FileManipulation/fileUtilities.h"
#include "../ResponseRelated/Integrity/Hash.h"
#include <cstddef>
#include <iostream>
#include <cstring>     
#include <unistd.h>    
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <string>

using std::string;

int main(){
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported\n";
        close(client_fd);
        return 1;
    }

    if (connect(client_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "Connection Failed\n";
        close(client_fd);
        return 1;
    }

    std::string input;
    char buffer[1024];
    size_t buffer_capacity = sizeof(buffer) - 1;

    while (true) {
        memset(buffer, 0, buffer_capacity + 1);
        ssize_t bytes_recvd = recv(client_fd, buffer, buffer_capacity, 0);
        if (bytes_recvd <= 0) {
            std::cout << "[Client] Connection closed.\n";
            break;
        }
        buffer[bytes_recvd] = '\0';
        std::cout << buffer;

        if (!std::getline(std::cin, input) || input.empty()) continue;

        std::pair<string, string> user_res = decoupledResponse(input);

        if(user_res.first == "USER" || user_res.first == "PASSWORD"){
            sendResponse(client_fd,input + "\n");
        }
        else if (user_res.first == "RETRIEVE") {
            sendResponse(client_fd, input + "\n");
            string user_path = "User_downloaded_files/" + getBaseName(input);
            upload(user_path);
        }   
        else if (user_res.first == "UPLOAD") {
            sendResponse(client_fd, input + "\n");
            ssize_t addr_bytes = recv(client_fd, buffer, buffer_capacity, 0);
            if (addr_bytes > 0) {
                buffer[addr_bytes] = '\0';
                string server_addr_info = string(buffer);
                
                size_t port_pos = server_addr_info.find(':');
                string ip = "127.0.0.1";
                int port = 8080;
                
                if (port_pos != string::npos) {
                    ip = server_addr_info.substr(0, port_pos);
                    port = std::stoi(server_addr_info.substr(port_pos + 1));
                }
                sockaddr_in udp_server{};
                udp_server.sin_family = AF_INET;
                udp_server.sin_port = htons(port);
                inet_pton(AF_INET, ip.c_str(), &udp_server.sin_addr);
                
                retrieve(user_res.second, udp_server, sizeof(udp_server));
            }
        }
        else if (user_res.first == "HASH"){
            sendResponse(client_fd, input + "\n");
            ssize_t hash = recv(client_fd,buffer,buffer_capacity,0);
            buffer[hash] = '\0';
            std::cerr<<"Hash of "+ user_res.second + " recieved\n";
            std::cout<< string(buffer) <<'\n';
        }
        else {
            sendResponse(client_fd, input + "\n");
            if (user_res.first == "QUIT" || input == "Exit") {
                break;
            }
        }
    }

    close(client_fd);
    return 0;
}