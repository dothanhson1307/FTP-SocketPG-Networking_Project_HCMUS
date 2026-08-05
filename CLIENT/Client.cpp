#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "Client/Client.h"
#include "Client/ClientHelper.h"
#include "Client/TransferClient.h"
#include "Helper/SocketIO.h"

using std::string;

int main() {
    int clientFd = socket(AF_INET, SOCK_STREAM, 0);

    if (clientFd < 0) {
        std::cerr << "[Client] Cannot create socket.\n";
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(CLIENT_CONTROL_PORT);

    if (
        inet_pton(
            AF_INET,
            "127.0.0.1",
            &serverAddress.sin_addr
        ) <= 0
    ) {
        std::cerr << "[Client] Invalid server IP.\n";
        close(clientFd);
        return 1;
    }

    if (
        connect(
            clientFd,
            reinterpret_cast<sockaddr*>(&serverAddress),
            sizeof(serverAddress)
        ) < 0
    ) {
        std::cerr << "[Client] Cannot connect to server.\n";
        close(clientFd);
        return 1;
    }

    string pendingData;
    string response;
    ClientSession session;

    if (!receiveLine(clientFd, pendingData, response)) {
        std::cerr << "[Client] Cannot receive server greeting.\n";
        close(clientFd);
        return 1;
    }

    std::cout << response;

    while (true) {
        response = "";
        
        std::cout << "ftp> ";

        string command;

        if (!std::getline(std::cin, command)) {
            break;
        }

        if (command.empty()) {
            continue;
        }

        if (handleTransferCommand(
                clientFd,
                serverAddress,
                pendingData,
                session,
                command
            )) {
            continue;
        }

        if (!sendAll(clientFd, command + "\r\n")) {
            std::cerr << "[Client] Send failed.\n";
            break;
        }

        if (!receiveLine(clientFd, pendingData, response)) {
            std::cerr << "[Client] Server closed the connection.\n";
            break;
        }

        std::cout << response;
        updateSessionAfterReply(command, response, session);

        if (response.compare(0, 3, "221") == 0) {
            break;
        }

        response = "";
    }

    close(clientFd);

    return 0;
}
