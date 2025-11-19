#pragma once

#include "Session.h"
#include <vector>
#include <chrono>

namespace tcp_server {

class HeartbeatManager {
public:
    explicit HeartbeatManager(int timeoutSeconds = 10);
    ~HeartbeatManager() = default;

    // Update heartbeat for a session
    void updateHeartbeat(SessionPtr session);

    // Check for timed out sessions
    std::vector<int> checkTimeouts(const std::vector<SessionPtr>& sessions);

    // Get timeout duration
    int getTimeoutSeconds() const { return timeoutSeconds_; }

private:
    int timeoutSeconds_;
};

using HeartbeatManagerPtr = std::shared_ptr<HeartbeatManager>;

} // namespace tcp_server
