#include "EpollServer.h"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

namespace tcp_server {

constexpr int MAX_EVENTS = 1024;
constexpr int BACKLOG = 128;

EpollServer::EpollServer(int port)
    : port_(port)
    , listenFd_(-1)
    , epollFd_(-1)
    , running_(false) {
}

EpollServer::~EpollServer() {
    stop();
}

bool EpollServer::createListenSocket() {
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }

    // Set SO_REUSEADDR
    int opt = 1;
    if (setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set SO_REUSEADDR: " << strerror(errno) << std::endl;
        close(listenFd_);
        return false;
    }

    // Bind
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(listenFd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind: " << strerror(errno) << std::endl;
        close(listenFd_);
        return false;
    }

    // Listen
    if (listen(listenFd_, BACKLOG) < 0) {
        std::cerr << "Failed to listen: " << strerror(errno) << std::endl;
        close(listenFd_);
        return false;
    }

    // Set non-blocking
    if (!setNonBlocking(listenFd_)) {
        close(listenFd_);
        return false;
    }

    std::cout << "Listening on port " << port_ << std::endl;
    return true;
}

bool EpollServer::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        std::cerr << "Failed to get flags: " << strerror(errno) << std::endl;
        return false;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "Failed to set non-blocking: " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

bool EpollServer::start() {
    if (running_) {
        return true;
    }

    if (!createListenSocket()) {
        return false;
    }

    // Create epoll
    epollFd_ = epoll_create1(0);
    if (epollFd_ < 0) {
        std::cerr << "Failed to create epoll: " << strerror(errno) << std::endl;
        close(listenFd_);
        return false;
    }

    // Add listen socket to epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listenFd_;
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, listenFd_, &ev) < 0) {
        std::cerr << "Failed to add listen socket to epoll: " 
                  << strerror(errno) << std::endl;
        close(epollFd_);
        close(listenFd_);
        return false;
    }

    running_ = true;
    std::cout << "Server started successfully" << std::endl;
    return true;
}

void EpollServer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    // Close all client connections
    for (auto& pair : sessions_) {
        close(pair.first);
    }
    sessions_.clear();

    if (epollFd_ >= 0) {
        close(epollFd_);
        epollFd_ = -1;
    }

    if (listenFd_ >= 0) {
        close(listenFd_);
        listenFd_ = -1;
    }

    std::cout << "Server stopped" << std::endl;
}

void EpollServer::runOnce(int timeoutMs) {
    if (!running_) {
        return;
    }

    struct epoll_event events[MAX_EVENTS];
    int nfds = epoll_wait(epollFd_, events, MAX_EVENTS, timeoutMs);

    if (nfds < 0) {
        if (errno == EINTR) {
            return;
        }
        std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
        return;
    }

    for (int i = 0; i < nfds; ++i) {
        int fd = events[i].data.fd;

        if (fd == listenFd_) {
            // New connection
            handleNewConnection();
        } else if (events[i].events & (EPOLLERR | EPOLLHUP)) {
            // Error or hangup
            handleClientDisconnect(fd);
        } else if (events[i].events & EPOLLIN) {
            // Data available
            handleClientData(fd);
        }
    }
}

void EpollServer::handleNewConnection() {
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(listenFd_, (struct sockaddr*)&clientAddr, &clientLen);

        if (clientFd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            std::cerr << "Accept error: " << strerror(errno) << std::endl;
            break;
        }

        // Set non-blocking
        if (!setNonBlocking(clientFd)) {
            close(clientFd);
            continue;
        }

        // Add to epoll
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;  // Edge-triggered
        ev.data.fd = clientFd;
        if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, clientFd, &ev) < 0) {
            std::cerr << "Failed to add client to epoll: " 
                      << strerror(errno) << std::endl;
            close(clientFd);
            continue;
        }

        // Create session
        auto session = std::make_shared<Session>(clientFd);
        sessions_[clientFd] = session;

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
        std::cout << "New connection from " << ip << ":" 
                  << ntohs(clientAddr.sin_port) 
                  << ", fd=" << clientFd << std::endl;

        if (newConnectionCb_) {
            newConnectionCb_(session);
        }
    }
}

void EpollServer::handleClientData(int fd) {
    auto it = sessions_.find(fd);
    if (it == sessions_.end()) {
        return;
    }

    auto session = it->second;
    char buffer[4096];

    while (true) {
        ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
        
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            std::cerr << "Recv error: " << strerror(errno) << std::endl;
            handleClientDisconnect(fd);
            return;
        } else if (n == 0) {
            // Connection closed
            handleClientDisconnect(fd);
            return;
        }

        // Append to buffer
        session->getBuffer().append(buffer, n);

        // Try to extract messages
        MessageHeader header;
        std::vector<char> body;
        while (session->getBuffer().extractMessage(header, body)) {
            if (messageCb_) {
                messageCb_(session, header, body);
            }
        }
    }
}

void EpollServer::handleClientDisconnect(int fd) {
    std::cout << "Client disconnected, fd=" << fd << std::endl;

    epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
    
    auto it = sessions_.find(fd);
    if (it != sessions_.end()) {
        sessions_.erase(it);
    }

    if (disconnectCb_) {
        disconnectCb_(fd);
    }
}

} // namespace tcp_server
