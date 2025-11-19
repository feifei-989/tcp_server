#pragma once

#include <cstdint>

namespace tcp_server {

// Magic number for packet validation
constexpr uint32_t PACKET_MAGIC = 0x12345678;

// Message types
enum class MessageType : uint16_t {
    UNKNOWN = 0,
    LOGIN_REQUEST = 1,
    LOGIN_RESPONSE = 2,
    HEARTBEAT = 3,
    DATA = 4,
    BROADCAST = 5,
    MAX_MESSAGE_TYPE = 100  // Maximum valid message type
};

// Message header structure
struct MessageHeader {
    uint32_t magic;        // Magic number for validation
    uint16_t type;         // Message type
    uint16_t reserved;     // Reserved for future use
    uint32_t totalLength;  // Total packet length (header + body)
    uint32_t bodyLength;   // Body data length
    
    MessageHeader() 
        : magic(PACKET_MAGIC)
        , type(0)
        , reserved(0)
        , totalLength(sizeof(MessageHeader))
        , bodyLength(0) {}
} __attribute__((packed));

// Minimum valid packet size
constexpr size_t MIN_PACKET_SIZE = sizeof(MessageHeader);

// Maximum packet size (16MB)
constexpr size_t MAX_PACKET_SIZE = 16 * 1024 * 1024;

// Maximum body size
constexpr size_t MAX_BODY_SIZE = MAX_PACKET_SIZE - sizeof(MessageHeader);

// Header validation result
enum class HeaderValidationResult {
    VALID,
    INVALID_MAGIC,
    INVALID_TYPE,
    INVALID_TOTAL_LENGTH,
    INVALID_BODY_LENGTH,
    LENGTH_MISMATCH
};

// Validate message header
inline HeaderValidationResult validateHeader(const MessageHeader& header) {
    // Check magic number
    if (header.magic != PACKET_MAGIC) {
        return HeaderValidationResult::INVALID_MAGIC;
    }

    // Check message type
    if (header.type == 0 || header.type > static_cast<uint16_t>(MessageType::MAX_MESSAGE_TYPE)) {
        return HeaderValidationResult::INVALID_TYPE;
    }

    // Check total length
    if (header.totalLength < sizeof(MessageHeader) || header.totalLength > MAX_PACKET_SIZE) {
        return HeaderValidationResult::INVALID_TOTAL_LENGTH;
    }

    // Check body length
    if (header.bodyLength > MAX_BODY_SIZE) {
        return HeaderValidationResult::INVALID_BODY_LENGTH;
    }

    // Check length consistency
    if (header.totalLength != sizeof(MessageHeader) + header.bodyLength) {
        return HeaderValidationResult::LENGTH_MISMATCH;
    }

    return HeaderValidationResult::VALID;
}

// Get validation error message
inline const char* getValidationErrorMessage(HeaderValidationResult result) {
    switch (result) {
        case HeaderValidationResult::VALID:
            return "Valid header";
        case HeaderValidationResult::INVALID_MAGIC:
            return "Invalid magic number";
        case HeaderValidationResult::INVALID_TYPE:
            return "Invalid message type";
        case HeaderValidationResult::INVALID_TOTAL_LENGTH:
            return "Invalid total length";
        case HeaderValidationResult::INVALID_BODY_LENGTH:
            return "Invalid body length";
        case HeaderValidationResult::LENGTH_MISMATCH:
            return "Length fields mismatch";
        default:
            return "Unknown error";
    }
}

// Login request body
struct LoginRequest {
    char username[32];
    char password[32];
} __attribute__((packed));

// Login response body
struct LoginResponse {
    uint32_t success;  // 1 = success, 0 = failure
    char message[64];
} __attribute__((packed));

} // namespace tcp_server
