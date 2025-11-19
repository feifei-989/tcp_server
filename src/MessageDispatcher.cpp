#include "MessageDispatcher.h"
#include <iostream>
#include <cstring>

namespace tcp_server {

MessageDispatcher::MessageDispatcher(SessionManagerPtr sessionMgr, 
                                    HeartbeatManagerPtr heartbeatMgr)
    : sessionMgr_(sessionMgr)
    , heartbeatMgr_(heartbeatMgr) {
}

void MessageDispatcher::dispatch(SessionPtr session, 
                                const MessageHeader& header,
                                const std::vector<char>& body) {
    MessageType type = static_cast<MessageType>(header.type);

    switch (type) {
        case MessageType::LOGIN_REQUEST:
            handleLoginRequest(session, body);
            break;

        case MessageType::HEARTBEAT:
            handleHeartbeat(session);
            break;

        case MessageType::DATA:
            handleDataMessage(session, body);
            break;

        default:
            std::cerr << "Unknown message type: " << header.type << std::endl;
            break;
    }
}

void MessageDispatcher::handleLoginRequest(SessionPtr session, 
                                          const std::vector<char>& body) {
    if (body.size() < sizeof(LoginRequest)) {
        std::cerr << "Invalid login request size" << std::endl;
        return;
    }

    LoginRequest req;
    std::memcpy(&req, body.data(), sizeof(LoginRequest));

    std::string username(req.username);
    std::string password(req.password);

    std::cout << "Login request from fd=" << session->getFd()
              << ", username=" << username << std::endl;

    // Simple authentication (in real system, check against database)
    bool success = !username.empty() && !password.empty();

    LoginResponse resp;
    resp.success = success ? 1 : 0;
    if (success) {
        session->setAuthenticated(true);
        session->setUsername(username);
        session->updateHeartbeat();
        std::strncpy(resp.message, "Login successful", sizeof(resp.message) - 1);
        std::cout << "User authenticated: " << username << std::endl;
    } else {
        std::strncpy(resp.message, "Login failed", sizeof(resp.message) - 1);
    }

    MessageHeader header;
    header.type = static_cast<uint16_t>(MessageType::LOGIN_RESPONSE);
    header.bodyLength = sizeof(LoginResponse);
    header.totalLength = sizeof(MessageHeader) + sizeof(LoginResponse);

    session->sendMessage(header, reinterpret_cast<const char*>(&resp));
}

void MessageDispatcher::handleHeartbeat(SessionPtr session) {
    if (!session->isAuthenticated()) {
        std::cerr << "Heartbeat from unauthenticated session, fd=" 
                  << session->getFd() << std::endl;
        return;
    }

    heartbeatMgr_->updateHeartbeat(session);
    
    // Echo heartbeat back
    MessageHeader header;
    header.type = static_cast<uint16_t>(MessageType::HEARTBEAT);
    header.bodyLength = 0;
    header.totalLength = sizeof(MessageHeader);
    
    session->sendMessage(header);
}

void MessageDispatcher::handleDataMessage(SessionPtr session, 
                                         const std::vector<char>& body) {
    if (!session->isAuthenticated()) {
        std::cerr << "Data message from unauthenticated session, fd=" 
                  << session->getFd() << std::endl;
        return;
    }

    std::string data(body.begin(), body.end());
    std::cout << "Data from " << session->getUsername() 
              << ": " << data << std::endl;

    // Echo back to sender
    MessageHeader header;
    header.type = static_cast<uint16_t>(MessageType::DATA);
    header.bodyLength = body.size();
    header.totalLength = sizeof(MessageHeader) + body.size();

    session->sendMessage(header, body.data());
}

} // namespace tcp_server
