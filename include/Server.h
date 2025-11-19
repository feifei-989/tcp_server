#pragma once

#include "EpollServer.h"
#include "SessionManager.h"
#include "HeartbeatManager.h"
#include "MessageDispatcher.h"
#include "ThreadPool.h"
#include <memory>
#include <thread>
#include <atomic>

namespace tcp_server {

class Server {
public:
    explicit Server(int port, int heartbeatTimeout = 10, size_t threadPoolSize = 4);
    ~Server();

    // Start the server
    bool start();

    // Stop the server
    void stop();

    // Run the server (blocking)
    void run();

    // Broadcast message to all authenticated clients
    void broadcast(const MessageHeader& header, const char* body = nullptr);

    // Send message to specific client
    bool sendToClient(int fd, const MessageHeader& header, const char* body = nullptr);

    // Get session count
    size_t getSessionCount() const;

    // Get thread pool stats
    size_t getPendingTaskCount() const;

private:
    void onNewConnection(SessionPtr session);
    void onMessage(SessionPtr session, const MessageHeader& header, 
                   const std::vector<char>& body);
    void onDisconnect(int fd);
    void heartbeatCheckLoop();

    int port_;
    std::atomic<bool> running_;

    EpollServerPtr epollServer_;
    SessionManagerPtr sessionMgr_;
    HeartbeatManagerPtr heartbeatMgr_;
    MessageDispatcherPtr dispatcher_;
    ThreadPoolPtr threadPool_;

    std::unique_ptr<std::thread> heartbeatThread_;
};

} // namespace tcp_server
