#include "ResponseHandling.h"
#include <cstddef>

void sendResponse(int client_fd, const string& response){
    ssize_t signal = send(client_fd, response.c_str(), response.length(), 0);
    if(signal > 0) std::cout<<"Sent successfully\n";
    else std::cerr<<"Failed to send response\n";
}

std::pair<string, string> decoupledResponse(const string& raw_request) {
    string request = raw_request;
    request.erase(request.find_last_not_of("\r\n") + 1);
    size_t space_pos = request.find(' ');
    if (space_pos != string::npos) {
        return { request.substr(0, space_pos), request.substr(space_pos + 1) };
    }
    return { request, "" };
}

std::pair<string,string> returnResponse(int client_fd, char* buffer, size_t buffer_capacity){

    string command;
    string argument;

    ssize_t bytes_received = recv(client_fd, buffer, buffer_capacity, 0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        string request(buffer);
        return decoupledResponse(request);
    } else {
        command = "EOF";
    }
    return std::pair<string,string>(command,argument);
}
