#include "Assistance.h"

#include "Helper/FtpReply.h"
#include "Helper/SocketIO.h"

#include <algorithm>
#include <cctype>
#include <unordered_map>

using std::string;

void handleHelp(const std::vector<string>& args, ServerSession& session) {
    if (args.size() == 1) {
        string helpMsg =
            "214-The following commands are recognized:\r\n"
            "   USER    PASS    NOOP    QUIT    PORT    PASV\r\n"
            "   TYPE    MODE    PWD     CWD     CDUP    MKD\r\n"
            "   RMD     LIST    NLST    STAT    SIZE    MDTM\r\n"
            "   RETR    STOR    APPE    STOU    ABOR    HASH\r\n"
            "   HELP\r\n"
            "214 Help OK.\r\n";
        sendAll(session.clientFd, helpMsg);
        return;
    }

    if (args.size() == 2) {
        string cmd = args[1];
        for (char& c : cmd) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }

        static const std::unordered_map<string, string> helpTopics = {
            {"USER", "214 Syntax: USER <username>\r\n"},
            {"PASS", "214 Syntax: PASS <password>\r\n"},
            {"NOOP", "214 Syntax: NOOP\r\n"},
            {"QUIT", "214 Syntax: QUIT\r\n"},
            {"PORT", "214 Syntax: PORT h1,h2,h3,h4,p1,p2\r\n"},
            {"PASV", "214 Syntax: PASV\r\n"},
            {"TYPE", "214 Syntax: TYPE <A|I>\r\n"},
            {"MODE", "214 Syntax: MODE <S|B|C>\r\n"},
            {"PWD",  "214 Syntax: PWD\r\n"},
            {"CWD",  "214 Syntax: CWD <directory-path>\r\n"},
            {"CDUP", "214 Syntax: CDUP\r\n"},
            {"MKD",  "214 Syntax: MKD <directory-name>\r\n"},
            {"RMD",  "214 Syntax: RMD <directory-name>\r\n"},
            {"LIST", "214 Syntax: LIST [<path>]\r\n"},
            {"NLST", "214 Syntax: NLST [<path>]\r\n"},
            {"STAT", "214 Syntax: STAT\r\n"},
            {"SIZE", "214 Syntax: SIZE <filename>\r\n"},
            {"MDTM", "214 Syntax: MDTM <filename>\r\n"},
            {"RETR", "214 Syntax: RETR <remote-filename>\r\n"},
            {"STOR", "214 Syntax: STOR <remote-filename>\r\n"},
            {"APPE", "214 Syntax: APPE <remote-filename>\r\n"},
            {"STOU", "214 Syntax: STOU [<filename>]\r\n"},
            {"ABOR", "214 Syntax: ABOR\r\n"},
            {"HASH", "214 Syntax: HASH <filename>\r\n"},
            {"HELP", "214 Syntax: HELP [<command>]\r\n"}
        };

        auto it = helpTopics.find(cmd);
        if (it != helpTopics.end()) {
            sendAll(session.clientFd, it->second);
        } else {
            sendAll(session.clientFd, "214 Syntax: " + cmd + "\r\n");
        }
        return;
    }

    sendAll(session.clientFd, ftpInvalidArguments());
}
