#include "DirectoryInformation.h"

#include "Helper/FtpReply.h"
#include "Helper/PathHelper.h"
#include "Helper/SocketIO.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

using std::string;

static bool isLoggedIn(const ServerSession& session) {
    return session.loggedIn && !session.homeDir.empty();
}

void handleList(const std::vector<string>& args, ServerSession& session) {
    if (args.size() < 1 || args.size() > 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    std::filesystem::path target;
    string input = args.size() == 2 ? args[1] : "";
    if (!resolvePathInsideHome(session, input, target)) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    if (!std::filesystem::is_directory(target)) {
        sendAll(session.clientFd, ftpDirectoryDoesNotExist());
        return;
    }

    // [Type][Perms] [Size] [Name]
    string respone;
    // loop through all level 1 files or folders in target directory
    // auto == std::filesystem::directory_entry not std::filesystem::path
    for(const auto& entry : std::filesystem::directory_iterator(target)) {
        string type, size, name;
        // is a folder?
        if(entry.is_directory()) {
            type = "d"; // "d" is a folder
            size = "0";
        }
        // is a file?
        else if (entry.is_regular_file()) {
            type = "-"; // "-" is a file
            size = std::to_string(entry.file_size());
        }

        name  = entry.path().filename().string();
        // tam thoi bo qua permission
        respone += type + " " + size + " bytes " + name + "\r\n";
    }

    sendAll(session.clientFd, respone);
}

void handleNlst(const std::vector<string>& args, ServerSession& session) {
    if (args.size() < 1 || args.size() > 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    std::filesystem::path target;
    string input = args.size() == 2 ? args[1] : "";
    if (!resolvePathInsideHome(session, input, target)) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    if (!std::filesystem::is_directory(target)) {
        sendAll(session.clientFd, ftpDirectoryDoesNotExist());
        return;
    }

    string response;
    for (const auto& entry : std::filesystem::directory_iterator(target)) {
        response += entry.path().filename().string() + "\r\n";
    }

    sendAll(session.clientFd, response);
}

void handleStat(const std::vector<string>& args, ServerSession& session) {
    if (args.size() < 1 || args.size() > 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    std::filesystem::path target;
    string input = args.size() == 2 ? args[1] : "";
    if (!resolvePathInsideHome(session, input, target)) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    if (!std::filesystem::exists(target)) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    string response = "211-Status follows\r\n";
    response += " Name: " + target.filename().string() + "\r\n";

    if (std::filesystem::is_directory(target)) {
        response += " Type: directory\r\n";
    } else if (std::filesystem::is_regular_file(target)) {
        response += " Type: file\r\n";
        response += " Size: " + std::to_string(std::filesystem::file_size(target)) + " bytes\r\n";
    }

    response += "211 End of status.\r\n";
    sendAll(session.clientFd, response);
}

void handleSize(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    std::filesystem::path target;
    if (!resolvePathInsideHome(session, args[1], target)) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    if (!std::filesystem::is_regular_file(target)) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    sendAll(session.clientFd, "213 " + std::to_string(std::filesystem::file_size(target)) + "\r\n");
}

void handleMdtm(const std::vector<string>& args, ServerSession& session) {
    if (args.size() != 2) {
        sendAll(session.clientFd, ftpInvalidArguments());
        return;
    }

    if (!isLoggedIn(session)) {
        sendAll(session.clientFd, ftpNotLoggedIn());
        return;
    }

    std::filesystem::path target;
    if (!resolvePathInsideHome(session, args[1], target)) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    if (!std::filesystem::is_regular_file(target)) {
        sendAll(session.clientFd, ftpFileUnavailable());
        return;
    }

    // get the file's last modified time.
    // last_write_time() uses the filesystem clock, so we convert it to system_clock to work with normal date and time values.
    auto fileTime = std::filesystem::last_write_time(target);
    auto systemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        fileTime - std::filesystem::file_time_type::clock::now()
        + std::chrono::system_clock::now()
    );
    
    // convert the time to UTC because the FTP MDTM command must return
    // time in this format: YYYYMMDDHHMMSS.
    // Example: 20260806143015.
    std::time_t time = std::chrono::system_clock::to_time_t(systemTime);
    std::tm utcTime{};
    gmtime_r(&time, &utcTime);

    std::ostringstream response;
    response << "213 " << std::put_time(&utcTime, "%Y%m%d%H%M%S") << "\r\n";
    sendAll(session.clientFd, response.str());
}
