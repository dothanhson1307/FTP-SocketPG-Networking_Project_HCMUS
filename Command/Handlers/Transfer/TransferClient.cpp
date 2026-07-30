#include "TransferClient.h"

#include "../../../Client.h"
#include "../../../Rdt_udp/RDT.h"
#include "../../../Helper/SocketHelper.h"


#include <arpa/inet.h>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

using std::string;

// split command and file directory
std::vector<string> tokenize(const string& command) {
    std::istringstream stream(command);
    std::vector<string> tokens;
    string token;
    while (stream >> token) {
        tokens.push_back(token);
    }

    if (!tokens.empty()) {
        for (char& character : tokens.front()) {
            character = static_cast<char>(
                std::toupper(static_cast<unsigned char>(character))
            );
        }
    }
    return tokens;
}

bool receiveAndPrintReply(int clientFd, string& pendingData, string& reply) {
    if (!receiveLine(clientFd, pendingData, reply)) {
        std::cerr << "[Client] Server closed the connection.\n";
        return false;
    }
    std::cout << reply;
    return true;
}

bool isPreliminaryReply(const string& reply) {
    return reply.rfind("150", 0) == 0;
}

int openReceiverSocket() {
    const int socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd < 0) {
        return -1;
    }

    int reuseAddress = 1;
    setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &reuseAddress, sizeof(reuseAddress));

    sockaddr_in receiverAddress{};
    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    receiverAddress.sin_port = htons(kDataPort);
    if (bind(
            socketFd,
            reinterpret_cast<const sockaddr*>(&receiverAddress),
            sizeof(receiverAddress)
        ) < 0) {
        close(socketFd);
        return -1;
    }
    return socketFd;
}

int openSenderSocket() {
    return socket(AF_INET, SOCK_DGRAM, 0);
}

bool sendCommandAndReceivePreliminaryReply(int clientFd, string& pendingData, const string& command) {
    if (!sendAll(clientFd, command + "\r\n")) {
        std::cerr << "[Client] Send failed.\n";
        return false;
    }

    string reply;
    return receiveAndPrintReply(clientFd, pendingData, reply) && isPreliminaryReply(reply);
}

bool handleTransferCommand(int clientFd, const sockaddr_in& serverAddress, string& pendingData, const ClientSession& session, const string& command) {
    const std::vector<string> arguments = tokenize(command);
    if (arguments.empty() || (arguments[0] != "RETR" && arguments[0] != "STOR")) {
        return false;
    }

    if (arguments.size() != 2) {
        std::cerr << "[Client] " << arguments[0] << " requires exactly one file path.\n";
        return true;
    }

    std::filesystem::path source;
    if (arguments[0] == "STOR") {
        if (!session.loggedIn || session.username.empty()) {
            std::cerr << "[Client] Login is required before STOR.\n";
            return true;
        }

        // STOR file being read from the user's local managed folder,
        // ex: user_data/kiet/Rumination.txt
        const std::filesystem::path requestedFile(arguments[1]);
        source = std::filesystem::path("user_data") /
                 session.username /
                 session.currentDir /
                 requestedFile.filename();

        std::error_code error;
        if (!std::filesystem::is_regular_file(source, error) || error) {
            std::cerr << "[Client] Local file is unavailable: "
                      << source.lexically_normal().string() << "\n";
            return true;
        }
    }

    if (!sendCommandAndReceivePreliminaryReply(clientFd, pendingData, command)) {
        return true;
    }

    if (arguments[0] == "RETR") {
        const std::filesystem::path outputDirectory = "client_data";
        const std::filesystem::path outputPath = outputDirectory / std::filesystem::path(arguments[1]).filename();
        std::error_code error;
        std::filesystem::create_directories(outputDirectory, error);
        if (error) {
            std::cerr << "[Client] Cannot create download directory.\n";
            return true;
        }

        const int udpSocket = openReceiverSocket();
        if (udpSocket < 0) {
            std::cerr << "[Client] Cannot open UDP receiver.\n";
            return true;
        }
        rdt_recv(udpSocket, outputPath.string());
        close(udpSocket);
    } else {
        const int udpSocket = openSenderSocket();
        if (udpSocket < 0) {
            std::cerr << "[Client] Cannot open UDP sender.\n";
            return true;
        }

        sockaddr_in dataServer = serverAddress;
        dataServer.sin_port = htons(kDataPort);
        rdt_send(
            udpSocket,
            source.string(),
            reinterpret_cast<const sockaddr*>(&dataServer),
            sizeof(dataServer)
        );
        close(udpSocket);
    }

    string completionReply;
    receiveAndPrintReply(clientFd, pendingData, completionReply);
    return true;
}
