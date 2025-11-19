#pragma once

#include "Session.h"
#include "SessionManager.h"
#include "MessageDispatcher.h"
#include <memory>
#include <functional>

namespace tcp_server {

class EpollServer {
public:
    using NewConnectionCallback = std::function<void(SessionPtr)>;
    using MessageCallback = std::function<void(SessionPtr, const MessageHeader&, 
                                               const std::vector<char>&)>;
    using DisconnectCallback = std::function<void(int)>;

    explicit EpollServer(int port);
    ~EpollServer();

    // Set callbacks
    void setNewConnectionCallback(NewConnectionCallback cb) { 
        newConnectionCb_ = cb; 
    }
    void setMessageCallback(MessageCallback cb) { 
        messageCb_ = cb; 
    }
    void setDisconnectCallback(DisconnectCallback cb) { 
        disconnectCb_ = cb; 
    }

    // Start the server
    bool start();

    // Stop the server
    void stop();

    // Run one iteration of event loop
    void runOnce(int timeoutMs = 100);

    // Close a client connection (can be called from upper layer)
    void closeConnection(int fd);

private:
    bool createListenSocket();
    bool setNonBlocking(int fd);
    void handleNewConnection();
    void handleClientData(int fd);
    void handleClientDisconnect(int fd);

    int port_;
    int listenFd_;
    int epollFd_;
    bool running_;

    std::map<int, SessionPtr> sessions_;

    NewConnectionCallback newConnectionCb_;
    MessageCallback messageCb_;
    DisconnectCallback disconnectCb_;
};

using EpollServerPtr = std::shared_ptr<EpollServer>;

} // namespace tcp_server
