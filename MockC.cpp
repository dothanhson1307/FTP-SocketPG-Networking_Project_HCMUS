#include <cstddef>
#include <iostream>
#include <cstring>     
#include <unistd.h>    
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <string>

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

    while(true){
        recv(client_fd,buffer,buffer_capacity,0);
        std::cout<<buffer;

        std::cin.ignore();
        std::getline(std::cin,input);
        ssize_t bytes_sent = send(client_fd,input.c_str(),input.length(),0);

        recv(client_fd,buffer,buffer_capacity,0);
        std::cout<<buffer;

        std::getline(std::cin,input);
        bytes_sent = send(client_fd,input.c_str(),input.length(),0);

        break;
    }

    close(client_fd);
    return 0;
}