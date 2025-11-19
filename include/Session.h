#pragma once

#include "PacketBuffer.h"
#include <memory>
#include <string>
#include <chrono>

namespace tcp_server {

class Session {
public:
    explicit Session(int fd);
    ~Session();

    // Get file descriptor
    int getFd() const { return fd_; }

    // Get packet buffer
    PacketBuffer& getBuffer() { return buffer_; }

    // Authentication state
    bool isAuthenticated() const { return authenticated_; }
    void setAuthenticated(bool auth) { authenticated_ = auth; }

    // Username
    const std::string& getUsername() const { return username_; }
    void setUsername(const std::string& name) { username_ = name; }

    // Last heartbeat time
    std::chrono::steady_clock::time_point getLastHeartbeat() const { 
        return lastHeartbeat_; 
    }
    void updateHeartbeat() { 
        lastHeartbeat_ = std::chrono::steady_clock::now(); 
    }

    // Send data
    bool send(const char* data, size_t len);
    bool sendMessage(const MessageHeader& header, const char* body = nullptr);

private:
    int fd_;
    PacketBuffer buffer_;
    bool authenticated_;
    std::string username_;
    std::chrono::steady_clock::time_point lastHeartbeat_;
};

using SessionPtr = std::shared_ptr<Session>;

} // namespace tcp_server
