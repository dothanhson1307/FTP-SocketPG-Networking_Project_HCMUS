CXX = g++
OPENSSL_PREFIX = $(shell brew --prefix openssl@3 2>/dev/null || echo /opt/homebrew/opt/openssl@3)
CXXFLAGS = -std=c++17 -Wall -I. -I$(OPENSSL_PREFIX)/include
LDFLAGS = -L$(OPENSSL_PREFIX)/lib -lcrypto -lpthread

CLIENT_SRCS = CLIENT/Client.cpp \
              CLIENT/ClientHelper.cpp \
              CLIENT/TransferClient.cpp \
              Architecture/RdtUdp/RDT.cpp \
              Helper/FtpReply.cpp \
              Helper/SocketIO.cpp \
              Command/Integrity/Hash.cpp

.PHONY: all clean client

all: client

client: $(CLIENT_SRCS)
	$(CXX) $(CXXFLAGS) $(CLIENT_SRCS) $(LDFLAGS) -o client_app

clean:
	rm -f client_app
