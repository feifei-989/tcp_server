#pragma once

#include "Session.h"
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace tcp_server {

class SessionManager {
public:
    SessionManager() = default;
    ~SessionManager() = default;

    // Add a new session
    void addSession(SessionPtr session);

    // Remove a session
    void removeSession(int fd);

    // Get session by fd
    SessionPtr getSession(int fd);

    // Get all authenticated sessions
    std::vector<SessionPtr> getAuthenticatedSessions();

    // Broadcast message to all authenticated clients
    void broadcast(const MessageHeader& header, const char* body = nullptr);

    // Send message to specific client
    bool sendToClient(int fd, const MessageHeader& header, const char* body = nullptr);

    // Get session count
    size_t getSessionCount() const;

private:
    std::map<int, SessionPtr> sessions_;
    mutable std::mutex mutex_;
};

using SessionManagerPtr = std::shared_ptr<SessionManager>;

} // namespace tcp_server
