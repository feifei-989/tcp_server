cmake_minimum_required(VERSION 3.10)
project(tcp_server)

# Set C++11 standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compile options
add_compile_options(-Wall -Wextra -O2)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/PacketBuffer.cpp
    src/Session.cpp
    src/SessionManager.cpp
    src/HeartbeatManager.cpp
    src/MessageDispatcher.cpp
    src/ThreadPool.cpp
    src/EpollServer.cpp
    src/Server.cpp
    src/main.cpp
)

# Create executable
add_executable(tcp_server ${SOURCES})

# Link pthread library
target_link_libraries(tcp_server pthread)

# Install target
install(TARGETS tcp_server DESTINATION bin)
