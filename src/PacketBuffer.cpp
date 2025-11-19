#include "PacketBuffer.h"
#include <cstring>
#include <iostream>

namespace tcp_server {

PacketBuffer::PacketBuffer() {
    buffer_.reserve(4096);
}

void PacketBuffer::append(const char* data, size_t len) {
    buffer_.insert(buffer_.end(), data, data + len);
}

bool PacketBuffer::extractMessage(MessageHeader& header, std::vector<char>& body) {
    // Need at least header size
    if (buffer_.size() < sizeof(MessageHeader)) {
        return false;
    }

    // Read header
    std::memcpy(&header, buffer_.data(), sizeof(MessageHeader));

    // Validate header using the validation function
    auto validationResult = validateHeader(header);
    if (validationResult != HeaderValidationResult::VALID) {
        std::cerr << "Header validation failed: " 
                  << getValidationErrorMessage(validationResult) << std::endl;
        std::cerr << "  Magic: 0x" << std::hex << header.magic << std::dec << std::endl;
        std::cerr << "  Type: " << header.type << std::endl;
        std::cerr << "  TotalLength: " << header.totalLength << std::endl;
        std::cerr << "  BodyLength: " << header.bodyLength << std::endl;
        
        // Clear buffer on validation error to prevent further issues
        buffer_.clear();
        return false;
    }

    // Check if we have complete packet
    if (buffer_.size() < header.totalLength) {
        // Not enough data yet, wait for more
        return false;
    }

    // Extract body
    if (header.bodyLength > 0) {
        body.resize(header.bodyLength);
        std::memcpy(body.data(), 
                   buffer_.data() + sizeof(MessageHeader), 
                   header.bodyLength);
    } else {
        body.clear();
    }

    // Remove processed packet from buffer
    buffer_.erase(buffer_.begin(), buffer_.begin() + header.totalLength);

    return true;
}

void PacketBuffer::clear() {
    buffer_.clear();
}

} // namespace tcp_server
