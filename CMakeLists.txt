cmake_minimum_required(VERSION 3.20)

project(HybridFTP LANGUAGES CXX)

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
    Command/Handlers/Directory/DirectoryCommands.cpp
    Command/Handlers/DirectoryInformation/DirectoryInformation.cpp
    Command/Handlers/ModeDictator/DictateMode.cpp
    Command/Handlers/Transfer/TransferCommands.cpp
    Command/Integrity/Hash.cpp
    Helper/FtpReply.cpp
    Helper/SocketIO.cpp
)

set(CLIENT_SOURCES
    CLIENT/Client.cpp
    CLIENT/ClientHelper.cpp
    CLIENT/TransferClient.cpp
    Architecture/RdtUdp/RDT.cpp
    Helper/FtpReply.cpp
    Helper/SocketIO.cpp
)

add_executable(server_app ${SERVER_SOURCES})
add_executable(client_app ${CLIENT_SOURCES})

foreach(target IN ITEMS server_app client_app)
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    target_link_libraries(${target} PRIVATE
        Threads::Threads
    )
endforeach()

target_link_libraries(server_app PRIVATE OpenSSL::Crypto)
