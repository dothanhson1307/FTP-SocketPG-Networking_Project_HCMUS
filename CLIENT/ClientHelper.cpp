#include "ClientHelper.h"

#include <cctype>
#include <sstream>
#include <sys/socket.h>

using std::string;

std::vector<string> tokenizeCommand(const string& command) {
    std::istringstream stream(command);
    std::vector<string> tokens;
    string token;

    while (stream >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

bool startsWithReplyCode(const string& response, const string& code) {
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
        if (startsWithReplyCode(response, "331")) {
            session.username = arguments[1];
            session.usernameAccepted = true;
            session.loggedIn = false;
        } else {
            session = ClientSession{};
        }
        return;
    }

    if (commandName == "PASS") {
        session.loggedIn = session.usernameAccepted && startsWithReplyCode(response, "230");
        return;
    }

    if (commandName == "PASV" && startsWithReplyCode(response, "227")) {
        size_t startParen = response.find('(');
        size_t endParen = response.find(')', startParen);
        if (startParen != string::npos && endParen != string::npos) {
            string csv = response.substr(startParen + 1, endParen - startParen - 1);
            std::stringstream ss(csv);
            string token;
            std::vector<int> parts;

            while (std::getline(ss, token, ',')) {
                try {
                    parts.push_back(std::stoi(token));
                } catch (...) {
                    break;
                }
            }

            if (parts.size() == 6) {
                session.dataPort = parts[4] * 256 + parts[5];
                session.isPassiveMode = true;
            }
        }
    }
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
