#include "TransferClient.h"

#include "Client/ClientHelper.h"
#include "Architecture/RdtUdp/RDT.h"
#include "Helper/SocketIO.h"
#include "Helper/FtpReply.h"


#include <arpa/inet.h>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <vector>

using std::string;

namespace {

//split commands into tokens for processing
std::vector<string> parseLineTokens(const string& line) {
    std::istringstream stream(line);
    std::vector<string> tokens;
    string token;

    while (stream >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

bool startsWithCode(const string& response, const string& code) {
    //rfind searches backwards
    return response.rfind(code, 0) == 0;
}

bool receiveReplyLine(
    int clientFd,
    string& pendingData,
    string& response
) {
    if (!receiveLine(clientFd, pendingData, response)) {
        std::cerr << "[Client] Server closed the connection.\n";
        return false;
    }

    std::cout << response;
    return true;
}

std::filesystem::path buildUserFilepath(

    const ClientSession& session,
    const string& rawFilename
) {
    std::filesystem::path filename = std::filesystem::path(rawFilename).filename();
    return std::filesystem::path("Repository/user_data") / session.username / filename;
}

void ensurePassiveDataChannel(
    int clientFd,
    string& pendingData,
    ClientSession& session
) {
    if (session.isPassiveMode && session.dataPort > 0) {
        return;
    }

    if (!sendAll(clientFd, "PASV\r\n")) {
        return;
    }

    string pasvResponse;
    if (receiveLine(clientFd, pendingData, pasvResponse)) {
        std::cout << pasvResponse;
        updateSessionAfterReply("PASV", pasvResponse, session);
    }
}

} // namespace


bool handleTransferCommand(
    int clientFd,
    const sockaddr_in& serverAddress,
    string& pendingData,
    const ClientSession& session,
    const string& rawLine
) {
    const std::vector<string> arguments = parseLineTokens(rawLine);
    if (arguments.empty()) {
        return false;
    }

    string commandName = arguments.front();
    for (char& character : commandName) {
        character = static_cast<char>(
            std::toupper(static_cast<unsigned char>(character))
        );
    }

    if (commandName == "RETR") {
        if (arguments.size() != 2) {
            return false;
        }
        //not authenticated
        if (!session.loggedIn || session.username.empty()) {
            std::cout << ftpNotLoggedIn();
            return true;
        }

        ensurePassiveDataChannel(clientFd, pendingData, const_cast<ClientSession&>(session));

        //send command for server to handle
        if (!sendAll(clientFd, rawLine + "\r\n")) {
            std::cerr << "[Client] Send failed.\n";
            return true;
        }


        string response;
        if (!receiveReplyLine(clientFd, pendingData, response)) {
            return true;
        }

        if (!startsWithCode(response, "125") && !startsWithCode(response, "150")) {
            return true;
        }

        std::filesystem::path savePath = buildUserFilepath(session, arguments[1]);
        std::error_code error;
        std::filesystem::create_directories(savePath.parent_path(), error);

        char serverIp[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &serverAddress.sin_addr, serverIp, sizeof(serverIp));

        rdtReceiveFile(savePath.string(), session.dataPort, serverIp);

        receiveReplyLine(clientFd, pendingData, response);
        return true;
    }

    if (commandName == "STOR" || commandName == "APPE") {
        if (arguments.size() != 2) {
            return false;
        }

        if (!session.loggedIn || session.username.empty()) {
            std::cout << ftpNotLoggedIn();
            return true;
        }

        std::filesystem::path uploadPath = buildUserFilepath(session, arguments[1]);
        std::error_code error;
        if (!std::filesystem::is_regular_file(uploadPath, error) || error) {
            std::cout << ftpFileUnavailable();
            return true;
        }

        ensurePassiveDataChannel(clientFd, pendingData, const_cast<ClientSession&>(session));

        if (!sendAll(clientFd, rawLine + "\r\n")) {
            std::cerr << "[Client] Send failed.\n";
            return true;
        }


        string response;
        if (!receiveReplyLine(clientFd, pendingData, response)) {
            return true;
        }

        if (!startsWithCode(response, "125") && !startsWithCode(response, "150")) {
            return true;
        }

        sockaddr_in serverUdpAddress = serverAddress;
        serverUdpAddress.sin_port = htons(session.dataPort);

        rdtSendFile(
            uploadPath.string(),
            serverUdpAddress,
            sizeof(serverUdpAddress)
        );

        receiveReplyLine(clientFd, pendingData, response);
        return true;
    }

    return false;
}
