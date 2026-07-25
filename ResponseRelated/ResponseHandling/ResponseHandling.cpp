#include "ResponseHandling.h"

void sendResponse(int client_fd, const string& response){
    ssize_t signal = send(client_fd, response.c_str(), response.length(), 0);
    if(signal > 0) std::cout<<"Sent successfully\n";
    else std::cerr<<"Failed to send response\n";
}

std::pair<string,string> returnResponse(int client_fd, char* buffer, size_t buffer_capacity){

    string command;
    string argument;

    ssize_t bytes_received = recv(client_fd, buffer, buffer_capacity, 0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        string request(buffer);
        request.erase(request.find_last_not_of("\r\n") + 1);
        std::cout << "[Server] Received: \"" << request << "\"\n";
        size_t space_pos = request.find(' ');

        if (space_pos != string::npos) {
            command = request.substr(0, space_pos);
            argument = request.substr(space_pos + 1);
        } else {
            command = request;
        }
    } else {
        command = "EOF";
    }
    return std::pair<string,string>(command,argument);
}