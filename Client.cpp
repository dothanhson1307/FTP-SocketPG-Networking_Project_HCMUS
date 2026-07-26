#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "Client.h"

using std::string;

// logic tuong tu ben server.cpp
bool sendAll(int socketFd, const string& message) {
    size_t totalSent = 0;

    while (totalSent < message.size()) {
        ssize_t sent = send(
            socketFd,
            message.data() + totalSent,
            message.size() - totalSent,
            0
        );

        if (sent <= 0) {
            return false;
        }

        totalSent += static_cast<size_t>(sent);
    }

    return true;
}

bool receiveLine(
    int socketFd,
    string& pendingData,
    string& response
) {
    while (true) {
        size_t newlinePosition = pendingData.find('\n');

        if (newlinePosition != string::npos) {
            response = pendingData.substr(0, newlinePosition + 1);
            pendingData.erase(0, newlinePosition + 1);

            return true;
        }

        char buffer[CLIENT_BUFFER_SIZE];

        ssize_t received = recv(
            socketFd,
            buffer,
            sizeof(buffer),
            0
        );

        if (received <= 0) {
            return false;
        }

        pendingData.append(buffer, static_cast<size_t>(received));
    }
}

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

    if (!receiveLine(clientFd, pendingData, response)) {
        std::cerr << "[Client] Cannot receive server greeting.\n";
        close(clientFd);
        return 1;
    }

    std::cout << response;

    while (true) {
        std::cout << "ftp> ";

        string command;

        if (!std::getline(std::cin, command)) {
            break;
        }

        if (command.empty()) {
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

        // neu code 221 xuat hien o dau respone (index 0, 1, 2) thi break
        if (response.compare(0, 3, "221") == 0) {
            break;
        }
    }

    close(clientFd);

    return 0;
}
