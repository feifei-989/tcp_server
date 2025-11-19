#include "Server.h"
#include "Protocol.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace tcp_server;

/**
 * 演示如何使用服务器 API 发送消息
 * 
 * 编译: g++ -std=c++11 -I../include server_api_demo.cpp -o server_api_demo
 * 
 * 注意: 这只是演示代码，实际使用时需要链接完整的服务器库
 */

void demonstrateServerAPI() {
    // 创建服务器实例
    Server server(8888, 10, 4);
    
    // 启动服务器
    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return;
    }

    // 在独立线程中运行服务器
    std::thread serverThread([&server]() {
        server.run();
    });

    // 等待一些客户端连接...
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // ========== 示例 1: 广播消息 ==========
    std::cout << "\n=== 示例 1: 广播消息给所有用户 ===" << std::endl;
    {
        MessageHeader header;
        header.type = static_cast<uint16_t>(MessageType::BROADCAST);
        std::string message = "服务器公告: 系统将在10分钟后维护";
        header.bodyLength = message.size();
        header.totalLength = sizeof(MessageHeader) + message.size();

        server.broadcast(header, message.c_str());
        std::cout << "广播消息已发送" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // ========== 示例 2: 通过 fd 发送消息 ==========
    std::cout << "\n=== 示例 2: 通过 fd 发送消息 ===" << std::endl;
    {
        int clientFd = 10;  // 假设这是某个客户端的 fd
        
        MessageHeader header;
        header.type = static_cast<uint16_t>(MessageType::DATA);
        std::string message = "这是发送给 fd=10 的私密消息";
        header.bodyLength = message.size();
        header.totalLength = sizeof(MessageHeader) + message.size();

        if (server.sendToClient(clientFd, header, message.c_str())) {
            std::cout << "消息已发送给 fd=" << clientFd << std::endl;
        } else {
            std::cout << "发送失败: 客户端 fd=" << clientFd << " 不存在或未登录" << std::endl;
        }
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // ========== 示例 3: 通过用户名发送消息 ==========
    std::cout << "\n=== 示例 3: 通过用户名发送消息 ===" << std::endl;
    {
        std::string username = "alice";
        
        MessageHeader header;
        header.type = static_cast<uint16_t>(MessageType::DATA);
        std::string message = "你好 Alice，这是发送给你的专属消息！";
        header.bodyLength = message.size();
        header.totalLength = sizeof(MessageHeader) + message.size();

        if (server.sendToUser(username, header, message.c_str())) {
            std::cout << "消息已发送给用户: " << username << std::endl;
        } else {
            std::cout << "发送失败: 用户 " << username << " 不存在或未登录" << std::endl;
        }
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // ========== 示例 4: 批量发送给多个用户 ==========
    std::cout << "\n=== 示例 4: 批量发送给多个指定用户 ===" << std::endl;
    {
        std::vector<std::string> targetUsers = {"alice", "bob", "charlie"};
        
        MessageHeader header;
        header.type = static_cast<uint16_t>(MessageType::DATA);
        std::string message = "重要通知: 请查收新的任务分配";
        header.bodyLength = message.size();
        header.totalLength = sizeof(MessageHeader) + message.size();

        int successCount = 0;
        for (const auto& username : targetUsers) {
            if (server.sendToUser(username, header, message.c_str())) {
                std::cout << "  ✓ 已发送给: " << username << std::endl;
                successCount++;
            } else {
                std::cout << "  ✗ 发送失败: " << username << std::endl;
            }
        }
        std::cout << "批量发送完成: " << successCount << "/" << targetUsers.size() << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // ========== 示例 5: 查询服务器状态 ==========
    std::cout << "\n=== 示例 5: 查询服务器状态 ===" << std::endl;
    {
        size_t sessionCount = server.getSessionCount();
        size_t pendingTasks = server.getPendingTaskCount();
        
        std::cout << "当前连接数: " << sessionCount << std::endl;
        std::cout << "待处理任务数: " << pendingTasks << std::endl;
    }

    // 停止服务器
    std::cout << "\n按 Enter 键停止服务器..." << std::endl;
    std::cin.get();
    
    server.stop();
    if (serverThread.joinable()) {
        serverThread.join();
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  TCP Server API 使用示例" << std::endl;
    std::cout << "========================================" << std::endl;

    demonstrateServerAPI();

    return 0;
}
