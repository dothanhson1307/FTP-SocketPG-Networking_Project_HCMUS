#include "Upload.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include "../../Rdt_udp/RDT/RDT.h"

void upload(const string& output_filepath) {
    int udp_server = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_server == -1) {
        std::cerr << "Failed to create UDP socket for upload\n";
        return;
    }

    rdt_recv(udp_server, output_filepath);

    close(udp_server);
}
