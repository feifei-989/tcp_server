#include "Session.h"
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

namespace tcp_server {

Session::Session(int fd)
    : fd_(fd)
    , authenticated_(false)
    , lastHeartbeat_(std::chrono::steady_clock::now()) {
}

Session::~Session() {
    // Note: fd_ is managed by EpollServer, not closed here
}

bool Session::send(const char* data, size_t len) {
    size_t totalSent = 0;
    while (totalSent < len) {
        ssize_t sent = ::send(fd_, data + totalSent, len - totalSent, MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            std::cerr << "Send error: " << strerror(errno) << std::endl;
            return false;
        }
        totalSent += sent;
    }
    return true;
}

bool Session::sendMessage(const MessageHeader& header, const char* body) {
    // Send header
    if (!send(reinterpret_cast<const char*>(&header), sizeof(header))) {
        return false;
    }

    // Send body if present
    if (body && header.bodyLength > 0) {
        if (!send(body, header.bodyLength)) {
            return false;
        }
    }

    return true;
}

} // namespace tcp_server
