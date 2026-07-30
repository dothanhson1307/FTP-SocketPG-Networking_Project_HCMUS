#include <arpa/inet.h>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include <sys/socket.h>
#include <netinet/in.h>

#include "Client.h"
#include "Helper/SocketHelper.h"
#include "Command/Handlers/Transfer/TransferClient.h"

using std::string;

namespace {

std::vector<string> tokenizeCommand(const string& command) {
    std::istringstream stream(command);
    std::vector<string> tokens;
    string token;

    while (stream >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

bool hasReplyCode(const string& response, const string& code) {
    return response.rfind(code, 0) == 0;
}

void updateSessionAfterReply(
    const string& command,
    const string& response,
    ClientSession& session
) {
    const std::vector<string> arguments = tokenizeCommand(command);
    if (arguments.empty()) {
        return;
    }

    string commandName = arguments.front();
    for (char& character : commandName) {
        character = static_cast<char>(
            std::toupper(static_cast<unsigned char>(character))
        );
    }

    if (commandName == "USER" && arguments.size() == 2) {
        if (hasReplyCode(response, "331")) {
            session.username = arguments[1];
            session.usernameAccepted = true;
            session.loggedIn = false;
        } else {
            session = ClientSession{};
        }
        return;
    }

    if (commandName == "PASS") {
        session.loggedIn = session.usernameAccepted && hasReplyCode(response, "230");
    }
}

} // namespace

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
    ClientSession session;

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

        // neu code 221 xuat hien o dau respone (index 0, 1, 2) thi break
        if (response.compare(0, 3, "221") == 0) {
            break;
        }
    }

    close(clientFd);

    return 0;
}
