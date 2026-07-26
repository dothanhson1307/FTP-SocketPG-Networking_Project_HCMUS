#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

#include <sys/socket.h>
#include <netinet/in.h>

#include "Server.h"
#include "Command/commandHandling.h"
#include "Command/command.h"

using std::string;

bool sendAll(int socketFd, const std::string& message) {
    size_t totalSent = 0;

    while (totalSent < message.size()) {
        ssize_t sent = send(
            socketFd,
            message.data() + totalSent,
            message.size() - totalSent,
            0
        );
        
        // xay ra khi viec send(..) bi loi: client quit/ disconnect/ invalid socket/ loi mang
        if (sent <= 0) {
            return false;
        }

        totalSent += static_cast<size_t>(sent);
    }

    return true;
}

int main() {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverFd < 0) {
        std::cerr << "[Server] Cannot create socket.\n";
        return 1;
    }

    int reuseAddress = 1;

    if (setsockopt(
            serverFd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &reuseAddress,
            sizeof(reuseAddress)
        ) < 0) {
        std::cerr << "[Server] setsockopt failed.\n";
        close(serverFd);
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(SERVER_CONTROL_PORT);

    if (bind(
            serverFd,
            reinterpret_cast<sockaddr*>(&serverAddress),
            sizeof(serverAddress)
        ) < 0) {
        std::cerr << "[Server] Bind failed.\n";
        close(serverFd);
        return 1;
    }
    
    // session
    if (listen(serverFd, 5) < 0) {
        std::cerr << "[Server] Listen failed.\n";
        close(serverFd);
        return 1;
    }

    std::cout << "[Server] Listening on port " << SERVER_CONTROL_PORT << "...\n";

    sockaddr_in clientAddress{};
    socklen_t clientAddressLength = sizeof(clientAddress);

    int clientFd = accept(
        serverFd,
        reinterpret_cast<sockaddr*>(&clientAddress),
        &clientAddressLength
    );

    if (clientFd < 0) {
        std::cerr << "[Server] Accept failed.\n";
        close(serverFd);
        return 1;
    }

    char clientIp[INET_ADDRSTRLEN]{}; // IPv4 dang thap phan ~ 16 ky tu

    inet_ntop(
        AF_INET,
        &clientAddress.sin_addr,
        clientIp,
        sizeof(clientIp)
    );

    std::cout << "[Server] Client connected: "
              << clientIp << "\n";

    if (!sendAll(clientFd, "220 Hybrid FTP server ready\r\n")) {
        close(clientFd);
        close(serverFd);
        return 1;
    }

    SessionState session;
    string pendingData;
    char buffer[SERVER_BUFFER_SIZE];

    bool running = true;

    while (running) {
        ssize_t received = recv(
            clientFd,
            buffer,
            sizeof(buffer),
            0
        );

        if (received == 0) {
            std::cout << "[Server] Client disconnected.\n";
            break;
        }

        if (received < 0) {
            std::cerr << "[Server] recv failed.\n";
            break;
        }

        pendingData.append(buffer, static_cast<size_t>(received));
        

        std::vector<std::vector<string>> commands;

        commands = read(pendingData);

        for (const auto& arguments : commands) {
            string response = executeCommand(arguments, session);

            if (!sendAll(clientFd, response)) {
                running = false;
                break;
            }

            if (session.quitRequested) {
                running = false;
                break;
            }
        }
    }

    close(clientFd);
    close(serverFd);

    std::cout << "[Server] Stopped.\n";

    return 0;
}
