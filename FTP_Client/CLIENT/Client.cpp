#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "CLIENT/Client.h"
#include "CLIENT/ClientHelper.h"
#include "CLIENT/TransferClient.h"
#include "Helper/SocketIO.h"

using std::string;

int main(int argc, char* argv[]) {
    string serverIp = "127.0.0.1";
    int serverPort = CLIENT_CONTROL_PORT;

    if (argc >= 2) {
        serverIp = argv[1];
    }
    if (argc >= 3) {
        try {
            serverPort = std::stoi(argv[2]);
        } catch (...) {
            std::cerr << "[Client] Invalid port specified. Using default: " << CLIENT_CONTROL_PORT << "\n";
            serverPort = CLIENT_CONTROL_PORT;
        }
    }

    std::cout << "[Client] Connecting to server at " << serverIp << ":" << serverPort << "...\n";

    int clientFd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientFd < 0) {
        std::cerr << "[Client] Cannot create socket.\n";
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIp.c_str(), &serverAddress.sin_addr) <= 0) {
        std::cerr << "[Client] Invalid server IP: " << serverIp << "\n";
        close(clientFd);
        return 1;
    }

    if (connect(clientFd, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) < 0) {
        std::cerr << "[Client] Cannot connect to server at " << serverIp << ":" << serverPort << ".\n";
        std::cerr << "         Please ensure the server is running and reachable across your network.\n";
        close(clientFd);
        return 1;
    }

    std::cout << "[Client] Connected successfully!\n";

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
