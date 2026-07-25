#include "Upload.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include "../../Rdt_udp/RDT/RDT.h"

void upload(const string& output_filepath) {
    int udp_client = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_client == -1) {
        std::cerr << "Failed to create UDP socket for upload\n";
        return;
    }

    int opt = 1;
    setsockopt(udp_client, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in client_udp_addr{};
    client_udp_addr.sin_family = AF_INET;
    client_udp_addr.sin_port = htons(8081);
    client_udp_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_client, reinterpret_cast<sockaddr*>(&client_udp_addr), sizeof(client_udp_addr)) < 0) {
        std::cerr << "Failed to bind client UDP socket to port 8081\n";
        close(udp_client);
        return;
    }

    rdt_recv(udp_client, output_filepath);

    close(udp_client);
}
