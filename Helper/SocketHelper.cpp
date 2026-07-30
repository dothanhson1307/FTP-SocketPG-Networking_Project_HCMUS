#include "SocketHelper.h"

#include <sys/socket.h>

using std::string;

bool sendAll(int socketFd, const string& message) {
    std::size_t totalSent = 0;

    while (totalSent < message.size()) {
        const ssize_t sent = send(
            socketFd,
            message.data() + totalSent,
            message.size() - totalSent,
            0
        );

        if (sent <= 0) {
            return false;
        }

        totalSent += static_cast<std::size_t>(sent);
    }

    return true;
}
