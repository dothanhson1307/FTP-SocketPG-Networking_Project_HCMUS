#include <iostream>
#include <cstring>     
#include <unistd.h>    
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <string>

int main() {
    // 1. CREATE SOCKET: Get our integer ticket (client_fd) from the OS
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        std::cerr << "[Client] Failed to create socket!\n";
        return -1;
    }

    // 2. CONFIGURE DESTINATION FORM: Where do we want to connect?
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080); // Target port 8080

    // Convert string IP ("127.0.0.1") into raw binary network byte order and store inside s_addr
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        std::cerr << "[Client] Invalid IP address format!\n";
        close(client_fd);
        return -1;
    }

    // 3. CONNECT: Trigger the TCP 3-Way Handshake across the network
    std::cout << "[Client] Attempting 3-way handshake with server at 127.0.0.1:8080...\n";
    if (connect(client_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "[Client] Connection refused! Is the server running?\n";
        close(client_fd);
        return -1;
    }
    std::cout << "[Client] Handshake successful! Connected.\n";

    // 4. INTERACTIVE COMMAND LOOP
    std::string userInput;
    char buffer[1024];
    size_t buffer_capacity = sizeof(buffer) - 1;

    std::cout << "[Client] Connected. Commands: USER <username>, PASS <password>, QUIT\n";

    while (true) {
        std::cout << "ftp> ";
        std::getline(std::cin, userInput);

        if (userInput.empty()) {
            continue;
        }

        // Send user input to server
        ssize_t bytes_sent = send(client_fd, userInput.c_str(), userInput.length(), 0);
        if (bytes_sent < 0) {
            std::cerr << "[Client] Failed to send message to server.\n";
            break;
        }

        // Wait for server response
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_received = recv(client_fd, buffer, buffer_capacity, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::cout << buffer; // Note: Server responses already end in \r\n
        } else if (bytes_received == 0) {
            std::cout << "[Client] Server closed the connection.\n";
            break;
        } else {
            std::cerr << "[Client] Error receiving data from server.\n";
            break;
        }

        // Exit loop if client sent QUIT
        if (userInput.rfind("QUIT", 0) == 0) {
            break;
        }
    }

    // 5. CLOSE SOCKET: Send TCP FIN packet to tell the server we are disconnecting
    close(client_fd);
    std::cout << "[Client] Disconnected from server. Goodbye.\n";
    return 0;
}