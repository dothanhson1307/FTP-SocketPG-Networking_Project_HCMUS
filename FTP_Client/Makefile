UNAME_S := $(shell uname -s)

CXX = g++
CXXFLAGS = -std=c++17 -Wall -I.

ifeq ($(UNAME_S),Darwin)
    OPENSSL_PREFIX = $(shell brew --prefix openssl@3 2>/dev/null || echo /opt/homebrew/opt/openssl@3)
    CXXFLAGS += -I$(OPENSSL_PREFIX)/include
    LDFLAGS = -L$(OPENSSL_PREFIX)/lib -lcrypto -lpthread
else
    # Linux / WSL Ubuntu
    HAS_OPENSSL := $(shell pkg-config --exists libcrypto 2>/dev/null && echo yes || (test -f /usr/include/openssl/sha.h && echo yes || echo no))
    ifeq ($(HAS_OPENSSL),yes)
        OPENSSL_CFLAGS = $(shell pkg-config --cflags libcrypto 2>/dev/null)
        OPENSSL_LIBS = $(shell pkg-config --libs libcrypto 2>/dev/null || echo -lcrypto)
        CXXFLAGS += $(OPENSSL_CFLAGS)
        LDFLAGS = $(OPENSSL_LIBS) -lpthread
    else
        LDFLAGS = -lpthread
    endif
endif

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
