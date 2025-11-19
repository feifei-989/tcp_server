#include "Server.h"
#include <iostream>
#include <chrono>

namespace tcp_server {

Server::Server(int port, int heartbeatTimeout, size_t threadPoolSize)
    : port_(port)
    , running_(false) {
    
    epollServer_ = std::make_shared<EpollServer>(port);
    sessionMgr_ = std::make_shared<SessionManager>();
    heartbeatMgr_ = std::make_shared<HeartbeatManager>(heartbeatTimeout);
    dispatcher_ = std::make_shared<MessageDispatcher>(sessionMgr_, heartbeatMgr_);
    threadPool_ = std::make_shared<ThreadPool>(threadPoolSize);

    std::cout << "Server initialized with thread pool size: " << threadPoolSize << std::endl;

    // Set callbacks
    epollServer_->setNewConnectionCallback(
        [this](SessionPtr session) { onNewConnection(session); });
    
    epollServer_->setMessageCallback(
        [this](SessionPtr session, const MessageHeader& header, 
               const std::vector<char>& body) {
            onMessage(session, header, body);
        });
    
    epollServer_->setDisconnectCallback(
        [this](int fd) { onDisconnect(fd); });
}

Server::~Server() {
    stop();
}

bool Server::start() {
    if (running_) {
        return true;
    }

    if (!epollServer_->start()) {
        return false;
    }

    running_ = true;

    // Start heartbeat check thread
    heartbeatThread_ = std::make_unique<std::thread>(
        [this]() { heartbeatCheckLoop(); });

    std::cout << "Server started on port " << port_ << std::endl;
    return true;
}

void Server::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    if (heartbeatThread_ && heartbeatThread_->joinable()) {
        heartbeatThread_->join();
    }

    epollServer_->stop();
    std::cout << "Server stopped" << std::endl;
}

void Server::run() {
    if (!running_) {
        std::cerr << "Server not started" << std::endl;
        return;
    }

    std::cout << "Server running, press Ctrl+C to stop" << std::endl;

    while (running_) {
        epollServer_->runOnce(100);
    }
}

void Server::broadcast(const MessageHeader& header, const char* body) {
    sessionMgr_->broadcast(header, body);
}

bool Server::sendToClient(int fd, const MessageHeader& header, const char* body) {
    return sessionMgr_->sendToClient(fd, header, body);
}

size_t Server::getSessionCount() const {
    return sessionMgr_->getSessionCount();
}

size_t Server::getPendingTaskCount() const {
    return threadPool_->getPendingTaskCount();
}

void Server::onNewConnection(SessionPtr session) {
    sessionMgr_->addSession(session);
}

void Server::onMessage(SessionPtr session, const MessageHeader& header,
                       const std::vector<char>& body) {
    // Submit message processing to thread pool
    // We need to capture copies of the data to avoid race conditions
    int fd = session->getFd();
    
    threadPool_->submit([this, session, header, body]() {
        // Process the message in worker thread
        try {
            dispatcher_->dispatch(session, header, body);
        } catch (const std::exception& e) {
            std::cerr << "Exception processing message from fd=" << session->getFd()
                     << ": " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception processing message from fd=" 
                     << session->getFd() << std::endl;
        }
    });
}

void Server::onDisconnect(int fd) {
    sessionMgr_->removeSession(fd);
}

void Server::heartbeatCheckLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Get all sessions and check for timeouts
        auto sessions = sessionMgr_->getAuthenticatedSessions();
        auto timedOutFds = heartbeatMgr_->checkTimeouts(sessions);

        // Remove timed out sessions
        for (int fd : timedOutFds) {
            std::cout << "Removing timed out session, fd=" << fd << std::endl;
            sessionMgr_->removeSession(fd);
            // Note: The actual socket close will happen in EpollServer
        }
    }
}

} // namespace tcp_server
