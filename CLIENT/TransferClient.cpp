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
#include <thread>
#include <vector>

using std::string;

namespace {

//split commands into tokens for processing
std::vector<string> parseCommandTokens(const string& rawLine) {
    std::istringstream stream(rawLine);
    std::vector<string> arguments;
    string argumentToken;

    while (stream >> argumentToken) {
        arguments.push_back(argumentToken);
    }

    return arguments;
}

bool startsWithCode(const string& responseLine, const string& replyCode) {
    return responseLine.rfind(replyCode, 0) == 0;
}

bool receiveReplyLine(
    int socketFd,
    string& pendingBuffer,
    string& outputReply
) {
    while (true) {
        size_t newlinePosition = pendingBuffer.find('\n');

        if (newlinePosition != string::npos) {
            outputReply = pendingBuffer.substr(0, newlinePosition + 1);
            pendingBuffer.erase(0, newlinePosition + 1);
            return true;
        }

        char buffer[CLIENT_BUFFER_SIZE];
        ssize_t bytesRead = recv(socketFd, buffer, sizeof(buffer), 0);

        if (bytesRead <= 0) {
            return false;
        }

        pendingBuffer.append(buffer, static_cast<size_t>(bytesRead));
    }
}

std::filesystem::path buildUserFilepath(
    const ClientSession& session,
    const string& rawFilename
) {
    std::filesystem::path filenamePath = std::filesystem::path(rawFilename).filename();

    return std::filesystem::absolute(
        std::filesystem::path("Repository/user_data")
        / session.username
        / filenamePath
    );
}

} // anonymous namespace

void ensurePassiveDataChannel(
    int clientFd,
    string& pendingData,
    ClientSession& session
) {
    if (!session.isPassiveMode) {
        return;
    }

    if (session.dataPort > 0) {
        return;
    }

    if (!sendAll(clientFd, "PASV\r\n")) {
        return;
    }

    string response;
    if (!receiveReplyLine(clientFd, pendingData, response)) {
        return;
    }

    std::cout << response;

    updateSessionAfterReply("PASV", response, session);
}


bool handleTransferCommand(
    int clientFd,
    const sockaddr_in& serverAddress,
    string& pendingData,
    const ClientSession& session,
    const string& rawLine
) {
    const std::vector<string> arguments = parseCommandTokens(rawLine);
    if (arguments.empty()) {
        return false;
    }

    string commandName = arguments.front();
    for (char& character : commandName) {
        character = static_cast<char>(
            std::toupper(static_cast<unsigned char>(character))
        );
    }

    if (commandName == "ABOR") {
        // The client has its own RDT thread.  Stop it as well as asking the
        // server to stop its thread; otherwise a receiver keeps waiting or a
        // sender keeps retrying packets after the server has aborted.
        if (arguments.size() == 1) {
            ClientSession& mutableSession = const_cast<ClientSession&>(session);
            if (mutableSession.isTransferring.load()) {
                mutableSession.abortRequested.store(true);
            }
        }

        // Let Client.cpp send ABOR and wait for the server's final reply.
        return false;
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

        std::cout << response;

        if (!startsWithCode(response, "125") && !startsWithCode(response, "150")) {
            return true;
        }

        std::filesystem::path savePath = buildUserFilepath(session, arguments[1]);
        std::error_code error;
        std::filesystem::create_directories(savePath.parent_path(), error);

        char serverIp[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &serverAddress.sin_addr, serverIp, sizeof(serverIp));

        ClientSession& mutableSession = const_cast<ClientSession&>(session);
        int port = session.dataPort;
        string ip = string(serverIp);
        char mode = session.transferMode;

        mutableSession.abortRequested.store(false);
        mutableSession.isTransferring.store(true);

        std::thread([savePath, port, ip, mode, &mutableSession, clientFd, &pendingData]() {
            rdtReceiveFile(savePath.string(), port, ip, false, &mutableSession.isTransferring, &mutableSession.abortRequested, mode);
            string completionReply;
            if (receiveReplyLine(clientFd, pendingData, completionReply)) {
                std::cout << "\r\033[K" << completionReply << "ftp> ";
                std::cout.flush();
            }

        }).detach();

        return true;
    }

    if (commandName == "STOR" || commandName == "APPE" || commandName == "STOU") {
        if (arguments.size() < 2 || arguments.size() > 2) {
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

        std::cout << response;

        if (!startsWithCode(response, "125") && !startsWithCode(response, "150")) {
            return true;
        }

        sockaddr_in serverUdpAddress = serverAddress;
        serverUdpAddress.sin_port = htons(session.dataPort);

        ClientSession& mutableSession = const_cast<ClientSession&>(session);
        char mode = session.transferMode;

        mutableSession.abortRequested.store(false);
        mutableSession.isTransferring.store(true);

        std::thread([uploadPath, serverUdpAddress, mode, &mutableSession, clientFd, &pendingData]() {
            rdtSendFile(
                uploadPath.string(),
                serverUdpAddress,
                sizeof(serverUdpAddress),
                &mutableSession.isTransferring,
                &mutableSession.abortRequested,
                mode
            );
            string completionReply;
            if (receiveReplyLine(clientFd, pendingData, completionReply)) {
                //\r jump to front 033 notify next is screen command,[K erase from cursor to end of the line
                std::cout << "\r\033[K" << completionReply << "ftp> ";
                std::cout.flush();
            }

        }).detach();

        return true;
    }

    return false;
}

