#include "SocketIO.h"

#include <cstddef>
#include <sys/socket.h>
#include <mutex>

static std::mutex mtx;

bool sendAll(int socketFd, const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx);
    size_t totalSent = 0;

    while (totalSent < message.size()) {
        ssize_t sent = send(
            socketFd,
            message.data() + totalSent,
            message.size() - totalSent,
            0
        );

        if (sent <= 0) {
            return false;
        }

        totalSent += static_cast<size_t>(sent);
    }
    return true;
}
