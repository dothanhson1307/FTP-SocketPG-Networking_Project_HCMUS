#include "CommandRouter.h"

#include "../Handlers/Authentication/AuthenticationCommands.h"
#include "../Handlers/Directory/DirectoryCommands.h"
#include "../Handlers/DirectoryInformation/DirectoryInformation.h"
#include "../Handlers/Transfer/TransferCommands.h"
#include "../Handlers/ModeDictator/DictateMode.h"
#include "../Integrity/IntegrityCommands.h"
#include "../../Helper/FtpReply.h"
#include "../../Helper/SocketIO.h"


#include <cctype>
#include <sstream>
#include <unordered_map>

using std::string;

std::vector<CommandArguments> extractCommandTokens(string& pendingData) {
    std::vector<CommandArguments> commands;

    while (true) {
        size_t newlinePosition = pendingData.find('\n');

        if (newlinePosition == string::npos) {
            break;
        }

        //skipped pass command
        string rawLine = pendingData.substr(0, newlinePosition);
        pendingData.erase(0, newlinePosition + 1);

        //trim '\r'
        size_t carriageReturnPosition = rawLine.find('\r');
        if (carriageReturnPosition != string::npos) {
            rawLine.erase(carriageReturnPosition, 1);
        }

        std::istringstream stream(rawLine);
        string token;
        if (!(stream >> token)) {
            continue;
        }

        for (char& character : token) {
            character = static_cast<char>(
                std::toupper(static_cast<unsigned char>(character))
            );
        }

        CommandArguments arguments{token};
        while (stream >> token) {
            arguments.push_back(token);
        }

        commands.push_back(arguments);
    }

    return commands;
}

void executeCommand(const CommandArguments& args, ServerSession& session) {
    if (args.empty()) {
        return;
    }

    static const std::unordered_map<string, CommandHandler> routes = {
        {"USER", handleUser},
        {"PASS", handlePass},
        {"NOOP", handleNoop},
        {"QUIT", handleQuit},

        {"PORT", handleActiveMode},
        {"PASV", handlePassiveMode},
        {"TYPE", handleType},
        {"MODE", handleMode},


        {"PWD",  handlePwd},
        {"CWD",  handleCwd},
        {"MKD",  handleMkd},
        {"RMD",  handleRmd},
        {"CDUP", handleCdup},

        {"LIST", handleList},
        {"NLST", handleNlst},
        {"STAT", handleStat},
        {"SIZE", handleSize},
        {"MDTM", handleMdtm},

        {"RETR", handleRetr},
        {"STOR", handleStor},
        {"APPE", handleAppe},
        {"STOU", handleStou},
        {"ABOR", handleAbort},
        {"HASH", handleHash},
        {"QUIT", handleQuit}
    };




    auto it = routes.find(args[0]);
    if (it != routes.end()) {
        it->second(args, session);
        return;
    }

    sendAll(session.clientFd, ftpCommandUnrecognized());
}
