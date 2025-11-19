#pragma once

#include "Protocol.h"
#include <vector>
#include <memory>

namespace tcp_server {

class PacketBuffer {
public:
    PacketBuffer();
    ~PacketBuffer() = default;

    // Append received data to buffer
    void append(const char* data, size_t len);

    // Try to extract a complete message from buffer
    // Returns true if a complete message is available
    bool extractMessage(MessageHeader& header, std::vector<char>& body);

    // Clear the buffer
    void clear();

    // Get current buffer size
    size_t size() const { return buffer_.size(); }

private:
    std::vector<char> buffer_;
};

} // namespace tcp_server
