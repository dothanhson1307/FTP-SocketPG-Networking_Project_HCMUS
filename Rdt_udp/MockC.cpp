#include <cstddef>
#include <iostream>
#include <cstring>     
#include <unistd.h>    
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <string>

#include "RDT/RDT.h"

using std::string;


int main(){
    int client_fd = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);

    if(inet_pton(AF_INET,"127.0.0.1",&server_addr.sin_addr)<0) std::cerr<<"can't bind address to pinging server\n";

    socklen_t server_len = sizeof(server_addr);
    char buffer[1024];
    size_t buffer_capacity = sizeof(buffer) -1;

    string input;
    while(true){
        memset(buffer,0,buffer_capacity+1);
        std::getline(std::cin,input);
        sendto(client_fd,input.c_str(),input.size(),0,reinterpret_cast<sockaddr*>(&server_addr),server_len);
        if(input == "Exit") break;
        if(input == "send file"){
            rdt_recv(client_fd,"endpoint.txt");
            continue;
        }
        sockaddr_in reply_addr{};
        socklen_t reply_len = sizeof(reply_addr);
        ssize_t r1 = recvfrom(client_fd, buffer, buffer_capacity, 0, reinterpret_cast<sockaddr*>(&reply_addr), &reply_len);
        if (r1 > 0) {
            buffer[r1] = '\0';
            std::cout << buffer;
        }
        ssize_t r2 = recvfrom(client_fd, buffer, buffer_capacity, 0, reinterpret_cast<sockaddr*>(&reply_addr), &reply_len);
        if (r2 > 0) {
            buffer[r2] = '\0';
            std::cout << buffer << "\n";
        }
    }

    close(client_fd);
    return 0;
}