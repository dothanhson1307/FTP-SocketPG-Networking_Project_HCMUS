cmake_minimum_required(VERSION 3.20)

project(FTP_Client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

set(CLIENT_SOURCES
    CLIENT/Client.cpp
    CLIENT/ClientHelper.cpp
    CLIENT/TransferClient.cpp

    Architecture/RdtUdp/RDT.cpp

    Command/Integrity/Hash.cpp

    Helper/FtpReply.cpp
    Helper/SocketIO.cpp
)

add_executable(client_app ${CLIENT_SOURCES})

target_include_directories(client_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(client_app PRIVATE
    Threads::Threads
    OpenSSL::Crypto
)

if(MSVC)
    target_compile_options(client_app PRIVATE /W4)
else()
    target_compile_options(client_app PRIVATE -Wall -Wextra -Wpedantic)
endif()
