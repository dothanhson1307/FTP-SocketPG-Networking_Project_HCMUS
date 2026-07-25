#include "Retrieve.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include "../../Rdt_udp/RDT/RDT.h"

void retrieve(const string& filepath, sockaddr_in client_addr, socklen_t client_len) {
    int udp_server = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_server == -1) {
        std::cerr << "Failed to create UDP socket for retrieve\n";
        return;
    }

    rdt_send(udp_server, filepath, reinterpret_cast<const sockaddr*>(&client_addr), client_len);
    
    close(udp_server);
}