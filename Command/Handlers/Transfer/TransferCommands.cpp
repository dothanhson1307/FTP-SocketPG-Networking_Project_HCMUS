#include "TransferCommands.h"

#include "../../../Rdt_udp/RDT.h"
#include "../../../ResponseRelated/Integrity/Hash.h"
#include "../../../Server.h"
#include "../../../Helper/FtpReply.h"
#include "../../../Helper/SocketHelper.h"

#include <arpa/inet.h>
#include <filesystem>
#include <sys/socket.h>
#include <unistd.h>

using std::string;

bool isLoggedIn(const ClientData& session) {
    return session.loggedIn && !session.homeDir.empty();
}

bool sendTransferStartReply(const ClientData& session, const string& reply) {
    return sendAll(session.clientFd, reply);
}

string handleRetr(const std::vector<string>& args, ClientData& session) {
    if (args.size() != 2) {
        return ftpInvalidArguments();
    }
    if (!isLoggedIn(session)) {
        return ftpNotLoggedIn();
    }

    if (args[1].empty()) {
        return ftpFileUnavailable();
    }

    const std::filesystem::path downloadableDirectory = std::filesystem::absolute("server_data/downloadable_files");
    const std::filesystem::path requestedFile = std::filesystem::path(args[1]).filename();
    std::filesystem::path source = downloadableDirectory / requestedFile;
    
    // check if the file is existed - file means *.exe, *.txt, ... not folder
    if (!std::filesystem::is_regular_file(source)) {
        return ftpFileUnavailable();
    }

    // send reply code and notification befor do transfer (download/upload both need)
    sendTransferStartReply(session, ftpOpeningDataConnection("RETR"));

    const int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        return ftpCannotOpenDataConnection();
    }

    sockaddr_in peer = session.clientAddress;
    peer.sin_port = htons(kDataPort); // Now was fixed = 8081

    rdt_send(
        udpSocket,
        source.string(),
        reinterpret_cast<const sockaddr*>(&peer),
        sizeof(peer)
    );
    close(udpSocket);

    return ftpTransferComplete();
}

string handleStor(const std::vector<string>& args, ClientData& session) {
    if (args.size() != 2) {
        return ftpInvalidArguments();
    }
    if (!isLoggedIn(session)) {
        return ftpNotLoggedIn();
    }

    if (args[1].empty()) {
        return ftpFileUnavailable();
    }

    /*
    Now i'm allowing client to upload files from "server_data/Uploaded_files".
    But, arcording to the project's require, we must just allow client to upload files from there space - 
    that is "user_data/username". So we need to refactor the code below.
    */

    // get file name *.exe, *.txt, ...
    std::filesystem::path filename = std::filesystem::path(args[1]).filename();
    std::filesystem::path destination = std::filesystem::absolute("server_data/Uploaded_files");
    // check if the folder is existed
    if(!std::filesystem::is_directory(destination / session.username)) {
        std::filesystem::create_directories(destination / session.username);
    }
    destination = destination / session.username;

    // check if the file is existed - file means *.exe, *.txt, ... not folder
    if (std::filesystem::is_regular_file(destination / filename)) {
        // handle duplicate file name later....
        return ftpFileAlreadyExists();
    }

    sendTransferStartReply(session, ftpOpeningDataConnection("STOR"));

    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        return ftpCannotOpenDataConnection();
    }

    int reuseAddress = 1;
    setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddress, sizeof(reuseAddress));
    
    sockaddr_in receiverAddress{};
    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    receiverAddress.sin_port = htons(kDataPort);
    if (bind(udpSocket, reinterpret_cast<const sockaddr*>(&receiverAddress), sizeof(receiverAddress)) < 0) {
        close(udpSocket);
        udpSocket = -1;
    }

    if (udpSocket < 0) {
        return ftpCannotOpenDataConnection();
    }

    rdt_recv(udpSocket, (destination / filename).string());
    // handle when file recieved is fail - return a reply
    close(udpSocket);

    return ftpTransferComplete();
}

string handleHash(const std::vector<string>& args, ClientData& session) {
    if (args.size() != 2) {
        return ftpInvalidArguments();
    }
    if (!isLoggedIn(session)) {
        return ftpNotLoggedIn();
    }

    std::filesystem::path source = session.homeDir / std::filesystem::path(args[1]).filename();
    source = source.lexically_normal();
    if (!std::filesystem::is_regular_file(source)) {
        return ftpFileUnavailable();
    }

    const string hash = calculateFileSHA256(source.string());
    if (hash.empty()) {
        return ftpFileUnavailable();
    }
    return ftpSha256(hash);
}
