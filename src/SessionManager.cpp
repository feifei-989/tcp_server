#include "SessionManager.h"
#include <iostream>

namespace tcp_server {

void SessionManager::addSession(SessionPtr session) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[session->getFd()] = session;
    std::cout << "Session added, fd=" << session->getFd() 
              << ", total sessions=" << sessions_.size() << std::endl;
}

void SessionManager::removeSession(int fd) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(fd);
    if (it != sessions_.end()) {
        std::cout << "Session removed, fd=" << fd;
        if (it->second->isAuthenticated()) {
            std::cout << ", user=" << it->second->getUsername();
        }
        std::cout << ", remaining sessions=" << (sessions_.size() - 1) << std::endl;
        sessions_.erase(it);
    }
}

SessionPtr SessionManager::getSession(int fd) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(fd);
    if (it != sessions_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<SessionPtr> SessionManager::getAuthenticatedSessions() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<SessionPtr> result;
    for (const auto& pair : sessions_) {
        if (pair.second->isAuthenticated()) {
            result.push_back(pair.second);
        }
    }
    return result;
}

void SessionManager::broadcast(const MessageHeader& header, const char* body) {
    auto sessions = getAuthenticatedSessions();
    std::cout << "Broadcasting to " << sessions.size() << " authenticated clients" << std::endl;
    
    for (const auto& session : sessions) {
        if (!session->sendMessage(header, body)) {
            std::cerr << "Failed to send to fd=" << session->getFd() << std::endl;
        }
    }
}

bool SessionManager::sendToClient(int fd, const MessageHeader& header, const char* body) {
    auto session = getSession(fd);
    if (!session) {
        std::cerr << "Session not found, fd=" << fd << std::endl;
        return false;
    }

    if (!session->isAuthenticated()) {
        std::cerr << "Session not authenticated, fd=" << fd << std::endl;
        return false;
    }

    return session->sendMessage(header, body);
}

bool SessionManager::sendToUser(const std::string& username, const MessageHeader& header, const char* body) {
    auto session = getSessionByUsername(username);
    if (!session) {
        std::cerr << "User not found: " << username << std::endl;
        return false;
    }

    if (!session->isAuthenticated()) {
        std::cerr << "User not authenticated: " << username << std::endl;
        return false;
    }

    std::cout << "Sending message to user: " << username << ", fd=" << session->getFd() << std::endl;
    return session->sendMessage(header, body);
}

SessionPtr SessionManager::getSessionByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& pair : sessions_) {
        if (pair.second->isAuthenticated() && 
            pair.second->getUsername() == username) {
            return pair.second;
        }
    }
    return nullptr;
}

size_t SessionManager::getSessionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}

} // namespace tcp_server
