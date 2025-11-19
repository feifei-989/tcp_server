#include "HeartbeatManager.h"
#include <iostream>

namespace tcp_server {

HeartbeatManager::HeartbeatManager(int timeoutSeconds)
    : timeoutSeconds_(timeoutSeconds) {
}

void HeartbeatManager::updateHeartbeat(SessionPtr session) {
    if (session) {
        session->updateHeartbeat();
    }
}

std::vector<int> HeartbeatManager::checkTimeouts(const std::vector<SessionPtr>& sessions) {
    std::vector<int> timedOutFds;
    auto now = std::chrono::steady_clock::now();

    for (const auto& session : sessions) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - session->getLastHeartbeat()).count();

        if (elapsed > timeoutSeconds_) {
            std::cout << "Session timeout detected, fd=" << session->getFd()
                     << ", elapsed=" << elapsed << "s" << std::endl;
            timedOutFds.push_back(session->getFd());
        }
    }

    return timedOutFds;
}

} // namespace tcp_server
