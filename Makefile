CXX = g++
OPENSSL_PREFIX = $(shell brew --prefix openssl@3 2>/dev/null || echo /opt/homebrew/opt/openssl@3)
CXXFLAGS = -std=c++17 -Wall -I. -I$(OPENSSL_PREFIX)/include
LDFLAGS = -L$(OPENSSL_PREFIX)/lib -lcrypto

SERVER_SRCS = SERVER/Server.cpp \
              Architecture/RdtUdp/RDT.cpp \
              Helper/FtpReply.cpp \
              Helper/SocketIO.cpp \
              Helper/PathHelper.cpp \
              Command/Router/CommandRouter.cpp \
              Command/Handlers/Authentication/AuthenticationCommands.cpp \
              Command/Handlers/Directory/DirectoryCommands.cpp \
              Command/Handlers/DirectoryInformation/DirectoryInformation.cpp \
              Command/Handlers/Transfer/TransferCommands.cpp \
              Command/Handlers/ModeDictator/DictateMode.cpp \
              Command/Handlers/Assistance/Assistance.cpp \
              Command/Handlers/FileOperation/FileOperationCommands.cpp \
              Command/Integrity/IntegrityCommands.cpp \
              Command/Integrity/Hash.cpp



CLIENT_SRCS = CLIENT/Client.cpp \
              CLIENT/ClientHelper.cpp \
              Architecture/RdtUdp/RDT.cpp \
              Helper/FtpReply.cpp \
              Helper/SocketIO.cpp \
              CLIENT/TransferClient.cpp \
              Command/Integrity/Hash.cpp


.PHONY: all clean server client

all: server client

server: $(SERVER_SRCS)
	$(CXX) $(CXXFLAGS) $(SERVER_SRCS) $(LDFLAGS) -o server_app

client: $(CLIENT_SRCS)
	$(CXX) $(CXXFLAGS) $(CLIENT_SRCS) $(LDFLAGS) -o client_app


clean:
	rm -f server_app client_app
