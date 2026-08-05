#pragma once

#include <string>

using std::string;

string ftpServiceReady();

string ftpCommandUnrecognized();
string ftpInvalidArguments();
string ftpCommandNotImplemented();
string ftpBadSequence();

string ftpUsernameAccepted();
string ftpLoginSuccessful();
string ftpNotLoggedIn();
string ftpCannotCreateUserDirectory();
string ftpGoodbye();

string ftpCurrentDirectory(const string& path);
string ftpDirectoryCreated();
string ftpCannotCreateDirectory();
string ftpDirectoryDoesNotExist();
string ftpCannotDeleteDirectory();
string ftpDirectoryDeleted();

string ftpOpeningDataConnection(const string& command);
string ftpCannotOpenDataConnection();
string ftpTransferAborted();
string ftpTransferComplete();
string ftpTransferAlreadyInProgress();


string ftpCommandSuccessful(const string& message);
string ftpPassiveMode(const string& ipCommas, int p1, int p2);

string ftpFileUnavailable();
string ftpFileAlreadyExists();
string ftpSha256(const string& digest);
