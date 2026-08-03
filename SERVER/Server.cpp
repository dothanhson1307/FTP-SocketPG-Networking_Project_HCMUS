#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <thread>
#include <mutex>

#include <sys/socket.h>
#include <netinet/in.h>

#include "Server.h"
#include "Command/Router/CommandRouter.h"
#include "Helper/FtpReply.h"
#include "Helper/SocketIO.h"

using std::string;

void handleNewClient(int clientFd,sockaddr_in clientAddress,socklen_t clientAddressLength){

    char clientIp[INET_ADDRSTRLEN]{};

    inet_ntop(
        AF_INET,
        &clientAddress.sin_addr,
        clientIp,
        sizeof(clientIp)
    );

    std::cout << "[Server] Client connected: "
              << clientIp << "\n";

    if (!sendAll(clientFd, ftpServiceReady())) {
        close(clientFd);
        
    }

    ServerSession session;
    session.clientAddress = clientAddress;
    session.clientFd = clientFd;
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

        commands = extractCommandTokens(pendingData);

        for (const auto& arguments : commands) {
            executeCommand(arguments, session);
            if (session.quitRequested) {
                running = false;
                break;
            }
        }
    }

    close(clientFd);
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

    if (::bind(
            serverFd,
            reinterpret_cast<sockaddr*>(&serverAddress),
            sizeof(serverAddress)
        ) < 0) {
        std::cerr << "[Server] Bind failed.\n";
        close(serverFd);
        return 1;
    }

    if (listen(serverFd, 5) < 0) {
        std::cerr << "[Server] Listen failed.\n";
        close(serverFd);
        return 1;
    }

    std::cout << "[Server] Listening on port " << SERVER_CONTROL_PORT << "...\n";

    while(true){
        sockaddr_in clientAddress{};
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientFd = accept(serverFd,reinterpret_cast<sockaddr*>(&clientAddress),&clientAddressLength);
        std::thread client_session(handleNewClient,clientFd,clientAddress,clientAddressLength);
        client_session.detach();
    }
    close(serverFd);

    std::cout << "[Server] Stopped.\n";

    return 0;
}
