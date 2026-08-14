#include "TransferClient.h"

#include "CLIENT/ClientHelper.h"
#include "Architecture/RdtUdp/RDT.h"
#include "Command/Integrity/Hash.h"
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

string fileKey(const string& rawFilename) {
    return std::filesystem::path(rawFilename).filename().string();
}

string extractReplyValue(const string& response) {
    if (response.size() < 4) {
        return "";
    }

    const size_t end = response.find_first_of("\r\n", 4);
    return response.substr(4, end == string::npos ? string::npos : end - 4);
}

string parseStouFilename(const string& response) {
    constexpr const char* prefix = "150 FILE: ";
    if (response.rfind(prefix, 0) != 0) {
        return "";
    }

    const size_t start = std::char_traits<char>::length(prefix);
    const size_t end = response.find_first_of("\r\n", start);
    return response.substr(start, end == string::npos ? string::npos : end - start);
}

void printVerificationResult(
    const string& sourceLabel,
    const string& sourceHash,
    const string& destinationLabel,
    const string& destinationHash
) {
    std::cout << sourceLabel << ": " << sourceHash << "\n";
    std::cout << destinationLabel << ": " << destinationHash << "\n";
    std::cout << "End-to-end verification: "
              << (sourceHash == destinationHash ? "MATCH" : "MISMATCH")
              << "\n";
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
        if (arguments.size() != 1) {
            return false;
        }

        ClientSession& mutableSession = const_cast<ClientSession&>(session);
        if (mutableSession.isTransferring.load()) {
            mutableSession.abortRequested.store(true);
            if (!sendAll(clientFd, rawLine + "\r\n")) {
                std::cerr << "[Client] Send failed.\n";
            }

            // The transfer worker already owns the next control reply and
            // will print the final 426 after UDP has stopped.
            return true;
        }

        if (mutableSession.completionReplyPending.load()) {
            std::cout << "450 Transfer is finalizing. Wait for its reply.\r\n";
            return true;
        }

        // With no transfer worker, Client.cpp can send ABOR and read the
        // normal 225 reply itself.
        return false;
    }

    if (commandName == "HASH") {
        if (arguments.size() != 2) {
            return false;
        }

        if (!session.loggedIn || session.username.empty()) {
            return false;
        }

        ClientSession& mutableSession = const_cast<ClientSession&>(session);
        if (mutableSession.isTransferring.load()
            || mutableSession.completionReplyPending.load()) {
            std::cout << "450 Transfer is still in progress. Try HASH after 226.\r\n";
            return true;
        }

        if (!sendAll(clientFd, rawLine + "\r\n")) {
            std::cerr << "[Client] Send failed.\n";
            return true;
        }

        string response;
        if (!receiveReplyLine(clientFd, pendingData, response)) {
            std::cerr << "[Client] Cannot receive HASH reply.\n";
            return true;
        }

        const string key = fileKey(arguments[1]);
        if (response.rfind("213 RETR-PRE ", 0) == 0) {
            const string serverPreHash = response.substr(
                std::char_traits<char>::length("213 RETR-PRE "),
                64
            );
            const string clientPostHash = calculateFileSHA256(
                buildUserFilepath(session, key).string()
            );

            if (clientPostHash.empty()) {
                std::cout << "End-to-end verification: local file is unavailable.\n";
            } else {
                printVerificationResult(
                    "Server pre-transfer SHA-256",
                    serverPreHash,
                    "Client post-transfer SHA-256",
                    clientPostHash
                );
            }
            return true;
        }

        if (startsWithCode(response, "213")) {
            string clientPreHash;
            {
                std::lock_guard<std::mutex> lock(mutableSession.verificationMutex);
                const auto savedHash = mutableSession.uploadSourceHashes.find(key);
                if (savedHash != mutableSession.uploadSourceHashes.end()) {
                    clientPreHash = savedHash->second;
                }
            }

            if (!clientPreHash.empty()) {
                printVerificationResult(
                    "Client pre-transfer SHA-256",
                    clientPreHash,
                    "Server post-transfer SHA-256",
                    extractReplyValue(response)
                );
            } else {
                // Keep the normal HASH behavior when this filename was not
                // uploaded by this client during the current session.
                std::cout << response;
            }

            return true;
        }

        // Server errors (for example 550) are still useful to show directly.
        std::cout << response;
        return true;
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
        mutableSession.completionReplyPending.store(true);

        std::thread([savePath, port, ip, mode, &mutableSession, clientFd, &pendingData]() {
            rdtReceiveFile(savePath.string(), port, ip, false, &mutableSession.isTransferring, &mutableSession.abortRequested, mode);
            string completionReply;
            if (receiveReplyLine(clientFd, pendingData, completionReply)) {
                std::cout << "\r\033[K" << completionReply << "ftp> ";
                std::cout.flush();
            }
            mutableSession.completionReplyPending.store(false);

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

        const bool shouldVerifyUpload = commandName == "STOR" || commandName == "STOU";
        const string clientPreHash = shouldVerifyUpload
            ? calculateFileSHA256(uploadPath.string())
            : "";
        if (shouldVerifyUpload && clientPreHash.empty()) {
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

        string remoteFilename = fileKey(arguments[1]);
        if (commandName == "STOU") {
            remoteFilename = parseStouFilename(response);
            if (remoteFilename.empty()) {
                std::cout << "[Client] Cannot determine STOU filename for verification.\n";
            }
        }

        sockaddr_in serverUdpAddress = serverAddress;
        serverUdpAddress.sin_port = htons(session.dataPort);

        ClientSession& mutableSession = const_cast<ClientSession&>(session);
        char mode = session.transferMode;

        mutableSession.abortRequested.store(false);
        mutableSession.isTransferring.store(true);
        mutableSession.completionReplyPending.store(true);

        std::thread([uploadPath, serverUdpAddress, mode, &mutableSession, clientFd, &pendingData,
                     shouldVerifyUpload, remoteFilename, clientPreHash]() {
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

                if (shouldVerifyUpload
                    && !remoteFilename.empty()
                    && startsWithCode(completionReply, "226")) {
                    std::lock_guard<std::mutex> lock(mutableSession.verificationMutex);
                    mutableSession.uploadSourceHashes[remoteFilename] = clientPreHash;
                }
            }
            mutableSession.completionReplyPending.store(false);

        }).detach();

        return true;
    }

    return false;
}
