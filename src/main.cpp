#include "Server.h"
#include <iostream>
#include <csignal>
#include <memory>

using namespace tcp_server;

std::unique_ptr<Server> g_server;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nReceived signal, shutting down..." << std::endl;
        if (g_server) {
            g_server->stop();
        }
    }
}

int main(int argc, char* argv[]) {
    // Default parameters
    int port = 8888;
    size_t threadPoolSize = 4;
    
    if (argc > 1) {
        port = std::atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Invalid port number" << std::endl;
            std::cerr << "Usage: " << argv[0] << " [port] [thread_pool_size]" << std::endl;
            std::cerr << "  port: 1-65535 (default: 8888)" << std::endl;
            std::cerr << "  thread_pool_size: number of worker threads (default: 4)" << std::endl;
            return 1;
        }
    }

    if (argc > 2) {
        threadPoolSize = std::atoi(argv[2]);
        if (threadPoolSize == 0) {
            threadPoolSize = 1;
        }
    }

    std::cout << "Starting TCP Server..." << std::endl;
    std::cout << "  Port: " << port << std::endl;
    std::cout << "  Thread Pool Size: " << threadPoolSize << std::endl;
    std::cout << "  Heartbeat Timeout: 10 seconds" << std::endl;

    // Setup signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Create and start server
    g_server = std::make_unique<Server>(port, 10, threadPoolSize);
    
    if (!g_server->start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    // Run server
    g_server->run();

    return 0;
}
