#include <cstddef>
#include <cstdint>
#include <iostream>
#include <cstring>     
#include <unistd.h>    
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include "User/User.h"
#include "RDT/RDT.h"



int main(){
    std::vector<User*> users_list = {new User("Son","1234"),new User("Kiet","1234")};

    int server_fd = socket(AF_INET,SOCK_DGRAM,0);
    if(server_fd == -1) std::cerr<<"Failed to create udp socket\n";

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);

    if(inet_pton(AF_INET,"127.0.0.1",&server_addr.sin_addr)<=0) std::cerr<<"failed to configure server's address";

    if(bind(server_fd,reinterpret_cast<sockaddr*>(&server_addr),sizeof(server_addr))<0) std::cerr<<"Failed to bind socket to address";

    // struct timeval tv;
    // tv.tv_sec = 2;
    // tv.tv_usec = 0;
    // setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    char buffer[1024];
    string response = "Message recieved: \n";

    while(true){

        sockaddr_in sender_addr{};
        socklen_t sender_len = sizeof(sender_addr);
        ssize_t bytes_received = recvfrom(server_fd,buffer,sizeof(buffer),0,reinterpret_cast<sockaddr*>(&sender_addr),&sender_len);
        if(bytes_received>0){
            std::cout<<"Server recieved\n";
            buffer[bytes_received] = '\0';
            string message = buffer;
            if(message == "Exit") break;
            if(message == "send file"){
                rdt_send(server_fd,"Downloadable files/daydreaming.txt",reinterpret_cast<sockaddr*>(&sender_addr),sender_len);
            }
            sendto(server_fd,response.c_str(),response.size(),0,reinterpret_cast<sockaddr*>(&sender_addr),sender_len);
            sendto(server_fd,buffer,bytes_received,0,reinterpret_cast<sockaddr*>(&sender_addr),sender_len);
        }

    }
    close(server_fd);


}