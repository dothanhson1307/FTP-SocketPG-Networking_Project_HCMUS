cmake_minimum_required(VERSION 3.20)

project(HybridFTP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

add_executable(server_app
    Server.cpp
    Command/Router/CommandRouter.cpp
    Command/Handlers/Authentication/AuthenticationCommands.cpp
    Command/Handlers/Directory/DirectoryCommands.cpp
    Command/Handlers/Transfer/TransferCommands.cpp
    Helper/FtpReply.cpp
    Helper/SocketHelper.cpp
    User/User.cpp
    Rdt_udp/RDT.cpp
    ResponseRelated/Integrity/Hash.cpp
)

target_compile_options(server_app PRIVATE
    -Wall
    -Wextra
)

target_link_libraries(server_app PRIVATE
    Threads::Threads
)

if(NOT APPLE)
    target_include_directories(server_app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/compat
    )
endif()

add_executable(client_app
    Client.cpp
    Command/Handlers/Transfer/TransferClient.cpp
    Helper/SocketHelper.cpp
    Rdt_udp/RDT.cpp
)

target_compile_options(client_app PRIVATE
    -Wall
    -Wextra
)

target_link_libraries(client_app PRIVATE
    Threads::Threads
)
