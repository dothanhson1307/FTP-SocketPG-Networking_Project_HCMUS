#include "FtpReply.h"

using std::string;

string ftpServiceReady() { return "220 Hybrid FTP server ready\r\n"; }

string ftpCommandUnrecognized() { return "500 Syntax error, command unrecognized!\r\n"; }
string ftpInvalidArguments() { return "501 Syntax error in parameters or arguments!\r\n"; }
string ftpCommandNotImplemented() { return "502 Command not implemented!\r\n"; }
string ftpBadSequence() { return "503 Bad sequence of commands!\r\n"; }

string ftpUsernameAccepted() { return "331 Username correct! Need password\r\n"; }
string ftpLoginSuccessful() { return "230 Login successfuly!\r\n"; }
string ftpNotLoggedIn() { return "530 Not logged in!\r\n"; }
string ftpCannotCreateUserDirectory() { return "550 Cannot create user directory!\r\n"; }
string ftpGoodbye() { return "221 Goodbye!\r\n"; }

string ftpCurrentDirectory(const string& path) { return "257 \"" + path + "\" is the current directory!\r\n"; }
string ftpDirectoryCreated() { return "257 Create new folder successfuly!\r\n"; }
string ftpCannotCreateDirectory() { return "550 Cannot create directory!\r\n"; }
string ftpDirectoryDoesNotExist() { return "550 Directory does not exist.\r\n"; }
string ftpCannotDeleteDirectory() { return "550 Cannot delete directory. It may not be empty.\r\n"; }
string ftpDirectoryDeleted() { return "250 Directory deleted successfully.\r\n"; }
string ftpChangeToParentDirectory() { return "250 Change the Client's working directory successfully.\r\n"; }
string ftpDirectoryChanged() {return "250 Directory changed successfully.\r\n";}

string ftpOpeningDataConnection(const string& command) { return "150 Opening UDP data connection for " + command + ".\r\n"; }
string ftpCannotOpenDataConnection() { return "425 Cannot open data connection!\r\n"; }
string ftpTransferAborted() { return "426 Connection closed; transfer aborted.\r\n"; }
string ftpTransferComplete() { return "226 Transfer complete.\r\n"; }

string ftpCommandSuccessful(const string& message) { return "200 " + message + "\r\n"; }
string ftpPassiveMode(const string& ipCommas, int p1, int p2) { return "227 Entering Passive Mode (" + ipCommas + "," + std::to_string(p1) + "," + std::to_string(p2) + ")\r\n"; }

string ftpFileUnavailable() { return "550 File unavailable.\r\n"; }
string ftpFileAlreadyExists() { return "550 File already exists.\r\n"; }
string ftpSha256(const string& digest) { return "213 " + digest + "\r\n"; }
