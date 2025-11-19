#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>

// Include protocol definition
#include "../include/Protocol.h"

using namespace tcp_server;

class TestClient {
public:
    TestClient(const std::string& host, int port)
        : host_(host), port_(port), sockFd_(-1), running_(false) {}

    ~TestClient() {
        disconnect();
    }

    bool connect() {
        sockFd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockFd_ < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }

        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        
        if (inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0) {
            std::cerr << "Invalid address" << std::endl;
            close(sockFd_);
            return false;
        }

        if (::connect(sockFd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Connection failed: " << strerror(errno) << std::endl;
            close(sockFd_);
            return false;
        }

        std::cout << "Connected to server" << std::endl;
        return true;
    }

    void disconnect() {
        running_ = false;
        if (sockFd_ >= 0) {
            close(sockFd_);
            sockFd_ = -1;
        }
    }

    bool login(const std::string& username, const std::string& password) {
        LoginRequest req;
        std::memset(&req, 0, sizeof(req));
        std::strncpy(req.username, username.c_str(), sizeof(req.username) - 1);
        std::strncpy(req.password, password.c_str(), sizeof(req.password) - 1);

        MessageHeader header;
        header.type = static_cast<uint16_t>(MessageType::LOGIN_REQUEST);
        header.bodyLength = sizeof(LoginRequest);
        header.totalLength = sizeof(MessageHeader) + sizeof(LoginRequest);

        if (!sendMessage(header, reinterpret_cast<const char*>(&req))) {
            return false;
        }

        // Wait for response
        MessageHeader respHeader;
        std::vector<char> respBody;
        if (!recvMessage(respHeader, respBody)) {
            return false;
        }

        if (respHeader.type != static_cast<uint16_t>(MessageType::LOGIN_RESPONSE)) {
            std::cerr << "Unexpected response type" << std::endl;
            return false;
        }

        LoginResponse resp;
        std::memcpy(&resp, respBody.data(), sizeof(resp));
        
        std::cout << "Login response: " << resp.message << std::endl;
        return resp.success == 1;
    }

    void startHeartbeat() {
        running_ = true;
        std::thread([this]() {
            while (running_) {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                
                MessageHeader header;
                header.type = static_cast<uint16_t>(MessageType::HEARTBEAT);
                header.bodyLength = 0;
                header.totalLength = sizeof(MessageHeader);

                if (!sendMessage(header)) {
                    std::cerr << "Failed to send heartbeat" << std::endl;
                    break;
                }

                std::cout << "Heartbeat sent" << std::endl;
            }
        }).detach();
    }

    bool sendData(const std::string& data) {
        MessageHeader header;
        header.type = static_cast<uint16_t>(MessageType::DATA);
        header.bodyLength = data.size();
        header.totalLength = sizeof(MessageHeader) + data.size();

        return sendMessage(header, data.c_str());
    }

private:
    bool sendMessage(const MessageHeader& header, const char* body = nullptr) {
        // Send header
        if (send(sockFd_, &header, sizeof(header), 0) != sizeof(header)) {
            std::cerr << "Failed to send header" << std::endl;
            return false;
        }

        // Send body if present
        if (body && header.bodyLength > 0) {
            if (send(sockFd_, body, header.bodyLength, 0) != (ssize_t)header.bodyLength) {
                std::cerr << "Failed to send body" << std::endl;
                return false;
            }
        }

        return true;
    }

    bool recvMessage(MessageHeader& header, std::vector<char>& body) {
        // Receive header
        if (recv(sockFd_, &header, sizeof(header), MSG_WAITALL) != sizeof(header)) {
            std::cerr << "Failed to receive header" << std::endl;
            return false;
        }

        // Receive body if present
        if (header.bodyLength > 0) {
            body.resize(header.bodyLength);
            if (recv(sockFd_, body.data(), header.bodyLength, MSG_WAITALL) 
                != (ssize_t)header.bodyLength) {
                std::cerr << "Failed to receive body" << std::endl;
                return false;
            }
        }

        return true;
    }

    std::string host_;
    int port_;
    int sockFd_;
    bool running_;
};

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 8888;

    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = std::atoi(argv[2]);
    }

    TestClient client(host, port);

    if (!client.connect()) {
        return 1;
    }

    // Login
    if (!client.login("testuser", "password123")) {
        std::cerr << "Login failed" << std::endl;
        return 1;
    }

    std::cout << "Login successful" << std::endl;

    // Start heartbeat
    client.startHeartbeat();

    // Send some test messages
    std::cout << "\nSending test messages..." << std::endl;
    for (int i = 0; i < 5; ++i) {
        std::string msg = "Test message " + std::to_string(i);
        if (client.sendData(msg)) {
            std::cout << "Sent: " << msg << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    std::cout << "\nPress Enter to disconnect..." << std::endl;
    std::cin.get();

    return 0;
}
