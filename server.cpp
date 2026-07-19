#include <iostream>
#include <cstring>      // For memset / strlen
#include <unistd.h>     // For close() and ssize_t
#include <sys/socket.h> // For socket(), bind(), listen(), accept(), send(), recv()
#include <netinet/in.h> // For sockaddr_in, htons(), INADDR_ANY
#include <arpa/inet.h>  // For inet_ntoa()

void sendResponse(int cliend_fd,const std::string& msg){
  
}

int main() {
    // 1. CREATE SOCKET: Get our integer ticket from the OS
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "[Server] Failed to create socket!\n";
        return -1;
    }

    // [Crucial Step]: Prevent "Address already in use" (TIME_WAIT) errors when restarting
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. CONFIGURE ADDRESS FORM: Zero-initialize and set IPv4, Port 8080, Any IP
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 3. BIND: Use reinterpret_cast to pass our specific IPv4 struct to the generic OS function
    if (bind(server_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "[Server] Bind failed! Is port 8080 already open?\n";
        close(server_fd);
        return -1;
    }

    // 4. LISTEN: Turn server_fd into a passive receptionist with a waiting room of 5
    listen(server_fd, 5);
    std::cout << "[Server] Receptionist active. Listening on port 8080...\n";

    // 5. ACCEPT: Wait for a client to knock, then create a brand new ticket (client_fd) just for them
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    
    std::cout << "[Server] Waiting for client connection (blocking)...\n";
    int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (client_fd < 0) {
        std::cerr << "[Server] Failed to accept client connection!\n";
        close(server_fd);
        return -1;
    }

    std::cout << "[Server] Client connected from IP: " << inet_ntoa(client_addr.sin_addr) << "\n";

    // 6. RECEIVE DATA: This is where size_t and ssize_t come into play!
    char buffer[1024]; 
    size_t buffer_capacity = sizeof(buffer) - 1; // size_t: Unsigned integer for memory capacities

    // ssize_t: Signed integer because recv() can return negative numbers for errors!
    ssize_t bytes_received = recv(client_fd, buffer, buffer_capacity, 0);

    // CONNECTING TO SECTION 4: Evaluating the 3 return states of ssize_t
    if (bytes_received > 0) {
        // STATE 1: Success! Data arrived from the kernel receive buffer.
        buffer[bytes_received] = '\0'; // Null-terminate the raw bytes so C++ can print it as a string
        std::cout << "[Server] Received " << bytes_received << " bytes: \"" << buffer << "\"\n";

        // 7. SEND DATA: Echo a reply back to the client
        const char* reply = "Hello from TCP Server! Your message was received.";
        size_t reply_length = strlen(reply); // size_t: Exactly how many bytes to push to the wire
        
        ssize_t bytes_sent = send(client_fd, reply, reply_length, 0);
        std::cout << "[Server] Sent " << bytes_sent << " bytes back to client.\n";

    } else if (bytes_received == 0) {
        // STATE 2: Exactly Zero = EOF / The client gracefully closed their socket
        std::cout << "[Server] Client abruptly closed the connection before sending data.\n";
    } else {
        // STATE 3: Negative One (-1) = Fatal networking error
        std::cerr << "[Server] Fatal error reading from socket!\n";
    }

    // 8. CLOSE SOCKETS: Release both tickets back to the OS
    close(client_fd); // Hang up on the specific client
    close(server_fd); // Shut down the main receptionist podium
    std::cout << "[Server] Sockets closed. Goodbye.\n";
    return 0;
}