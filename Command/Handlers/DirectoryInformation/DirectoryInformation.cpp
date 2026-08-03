#include "DirectoryInformation.h"
#include "Helper/FtpReply.h"

#include <filesystem>
using std::string;

static bool isLoggedIn(const ServerSession& session) {
    return session.loggedIn && !session.homeDir.empty();
}

string handleList(const std::vector<string>& args, ServerSession& session) {
    if (args.size() < 1 || args.size() > 2) {
        return ftpInvalidArguments();
    }

    if (!isLoggedIn(session)) {
        return ftpNotLoggedIn();
    }
    
    string targetDirectory;
    if (args.size() == 1) {
        targetDirectory = (session.homeDir.string() + "/" + session.currentDir.string());
    }
    else {
        if(!std::filesystem::exists(args[1]) && !std::filesystem::is_directory(args[1])) {
            return ftpDirectoryDoesNotExist();
        }

        targetDirectory = args[1];
    }
    // [Type][Perms] [Size] [Name]
    string respone = "";
    // loop through all level 1 files or folders in target directory
    // auto == std::filesystem::directory_entry not std::filesystem::path
    for(const auto& entry : std::filesystem::directory_iterator(targetDirectory)) {
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

    return respone;
}