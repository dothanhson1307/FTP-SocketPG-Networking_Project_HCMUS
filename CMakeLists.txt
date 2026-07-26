cmake_minimum_required(VERSION 3.20)

project(HybridFTP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

add_executable(server_app
    Server.cpp
    Command/commandHandling.cpp
    Command/command.cpp
    User/User.cpp
)

target_compile_options(server_app PRIVATE
    -Wall
    -Wextra
)

target_link_libraries(server_app PRIVATE
    Threads::Threads
)

add_executable(client_app
    Client.cpp
)

target_compile_options(client_app PRIVATE
    -Wall
    -Wextra
)

target_link_libraries(client_app PRIVATE
    Threads::Threads
)