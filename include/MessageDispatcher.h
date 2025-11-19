#pragma once

#include "Protocol.h"
#include "Session.h"
#include "SessionManager.h"
#include "HeartbeatManager.h"
#include <memory>
#include <vector>

namespace tcp_server {

class MessageDispatcher {
public:
    MessageDispatcher(SessionManagerPtr sessionMgr, HeartbeatManagerPtr heartbeatMgr);
    ~MessageDispatcher() = default;

    // Dispatch a message to appropriate handler
    void dispatch(SessionPtr session, const MessageHeader& header, 
                 const std::vector<char>& body);

private:
    void handleLoginRequest(SessionPtr session, const std::vector<char>& body);
    void handleHeartbeat(SessionPtr session);
    void handleDataMessage(SessionPtr session, const std::vector<char>& body);

    SessionManagerPtr sessionMgr_;
    HeartbeatManagerPtr heartbeatMgr_;
};

using MessageDispatcherPtr = std::shared_ptr<MessageDispatcher>;

} // namespace tcp_server
