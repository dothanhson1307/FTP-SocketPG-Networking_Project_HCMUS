# 🏗️ Architecture & Naming Refactoring Summary

This document details all refactoring changes made across the codebase to eliminate misleading terminology, fix inverted network function names, unify naming conventions, and improve architectural clarity.

---

## 1. Directory & File Organization Refactorings

| Previous Path | New Refactored Path | Technical Rationale |
|---|---|---|
| `Command/Handlers/Transfer/TransferClient.cpp` | `Client/TransferClient.cpp` | **Client/Server Separation**: `TransferClient.cpp` handles client-side UDP commands. Moving it to `Client/` prevents client code from residing in the server command router handlers directory. |
| `Architecture/Rdt_udp/` | `Architecture/RdtUdp/` | **Consistent Naming**: Aligns directory naming with PascalCase (`Architecture`, `Session`, `Command`, `Handlers`). |
| `Architecture/Session/ClientData.h` | `Architecture/Session/ServerSession.h` | **Domain Clarity**: Renamed `struct ClientData` to `struct ServerSession` to accurately reflect its role as the active server-side session container. |
| `Helper/SocketHelper.h` | `Helper/SocketIO.h` | **Specific Scope**: Replaced generic "Helper" terminology with explicit network I/O domain naming. |

---

## 2. Inverted & Misleading Method Renamings

| Previous Method Name | New Refactored Method Name | Location | Technical Rationale |
|---|---|---|---|
| `read(pendingData)` | `extractCommandTokens(pendingData)` | `CommandRouter.h` | **Eliminate I/O Confusion**: `read()` sounded like a socket/file read call. `extractCommandTokens()` correctly describes string parsing & tokenization in memory. |
| `upload(filename)` | `rdtReceiveFile(filename)` | `RDT.h / RDT.cpp` | **Fix Inverted Name**: `upload()` opened `ofstream` and received UDP packets. `rdtReceiveFile()` correctly describes the payload direction. |
| `retrieve(filename, addr)` | `rdtSendFile(filename, addr)` | `RDT.h / RDT.cpp` | **Fix Inverted Name**: `retrieve()` opened `ifstream` and transmitted UDP packets. `rdtSendFile()` accurately reflects packet transmission. |
| `hasReplyCode()` | `startsWithReplyCode()` | `Client.cpp` | **Exact Condition**: `hasReplyCode()` sounded like substring search. `startsWithReplyCode()` clarifies index-0 prefix matching. |
| `receiveLine()` | `extractResponseLineFromSocket()` | `Client.cpp` | **Buffer Accumulator Clarity**: Clarifies stream buffer frame extraction across TCP chunk boundaries. |
| `sendTransferStartReply()` | `sendSessionReply()` | `TransferCommands.cpp` | **Remove Misleading Prefix**: Replaced misleading transfer-specific name with generic session reply wrapper. |

---

## 3. Class & Struct Type Renamings

| Previous Type Name | New Refactored Type Name | Location | Technical Rationale |
|---|---|---|---|
| `struct ClientData` | `struct ServerSession` | `ServerSession.h` | Clarifies server-side session container vs local `ClientSession`. |

---

## 4. Updated Compilation (`Makefile`)

The `Makefile` has been updated to reference all refactored source locations and binaries:

```makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I.

SERVER_SRCS = Server/Server.cpp \
              Architecture/RdtUdp/RDT.cpp \
              Helper/FtpReply.cpp \
              Helper/SocketIO.cpp \
              Command/Router/CommandRouter.cpp \
              Command/Handlers/Authentication/AuthenticationCommands.cpp \
              Command/Handlers/Directory/DirectoryCommands.cpp \
              Command/Handlers/Transfer/TransferCommands.cpp \
              Command/Integrity/Hash.cpp

CLIENT_SRCS = Client/Client.cpp \
              Architecture/RdtUdp/RDT.cpp \
              Helper/SocketIO.cpp \
              Client/TransferClient.cpp \
              Command/Integrity/Hash.cpp

.PHONY: all clean server client

all: server client

server: $(SERVER_SRCS)
	$(CXX) $(CXXFLAGS) $(SERVER_SRCS) -o server_app

client: $(CLIENT_SRCS)
	$(CXX) $(CXXFLAGS) $(CLIENT_SRCS) -o client_app

clean:
	rm -f server_app client_app
```
