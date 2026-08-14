cmake_minimum_required(VERSION 3.20)

project(FTP_Server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

set(SERVER_SOURCES
    SERVER/Server.cpp

    Architecture/RdtUdp/RDT.cpp

    Command/Router/CommandRouter.cpp
    Command/Handlers/Authentication/AuthenticationCommands.cpp
    Command/Handlers/Assistance/Assistance.cpp
    Command/Handlers/Directory/DirectoryCommands.cpp
    Command/Handlers/DirectoryInformation/DirectoryInformation.cpp
    Command/Handlers/FileOperation/FileOperationCommands.cpp
    Command/Handlers/ModeDictator/DictateMode.cpp
    Command/Handlers/Transfer/TransferCommands.cpp
    Command/Integrity/Hash.cpp
    Command/Integrity/IntegrityCommands.cpp

    Helper/FtpReply.cpp
    Helper/PathHelper.cpp
    Helper/SocketIO.cpp
)

add_executable(server_app ${SERVER_SOURCES})

target_include_directories(server_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(server_app PRIVATE
    Threads::Threads
    OpenSSL::Crypto
)

if(MSVC)
    target_compile_options(server_app PRIVATE /W4)
else()
    target_compile_options(server_app PRIVATE -Wall -Wextra -Wpedantic)
endif()
