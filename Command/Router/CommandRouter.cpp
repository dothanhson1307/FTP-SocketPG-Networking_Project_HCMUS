#include "CommandRouter.h"

#include "../Handlers/Authentication/AuthenticationCommands.h"
#include "../Handlers/Directory/DirectoryCommands.h"
#include "../Handlers/Transfer/TransferCommands.h"
#include "../../Helper/FtpReply.h"

#include <cctype>
#include <sstream>
#include <unordered_map>

using std::string;

std::vector<CommandArguments> read(string& pendingData) {
    std::vector<CommandArguments> commands;
    std::size_t newLinePosition;

    while ((newLinePosition = pendingData.find('\n')) != string::npos) {
        string line = pendingData.substr(0, newLinePosition);
        pendingData.erase(0, newLinePosition + 1);

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::istringstream lineStream(line);
        string token;
        if (!(lineStream >> token)) {
            continue;
        }

        for (char& character : token) {
            character = static_cast<char>(
                std::toupper(static_cast<unsigned char>(character))
            );
        }

        CommandArguments arguments{token};
        while (lineStream >> token) {
            arguments.push_back(token);
        }
        commands.push_back(arguments);
    }

    return commands;
}

static const std::unordered_map<string, CommandHandler> kRouter = {
    {"USER", handleUser},
    {"PASS", handlePass},
    {"QUIT", handleQuit},
    {"PWD", handlePwd},
    {"MKD", handleMkd},
    {"RMD", handleRmd},
    {"RETR", handleRetr},
    {"STOR", handleStor},
    {"HASH", handleHash}
};

string executeCommand(const CommandArguments& args, ClientData& session) {
    if (args.empty()) {
        return ftpCommandUnrecognized();
    }

    const auto iterator = kRouter.find(args[0]);
    if (iterator == kRouter.end()) {
        return ftpCommandNotImplemented();
    }

    return iterator->second(args, session);
}
