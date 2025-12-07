// File: ShowcaseProtocol.h
// Project: StationX Showcase Network Protocol
// Purpose: Shared message definitions and utility helpers used by all stations
// Notes:
//  - This header is intentionally simple and portable across ESP32-based stations.
//  - Messages are packed to a fixed size to ensure stable behavior over ESP-NOW.
//  - Non-functional, documentation-only changes made on 2025-12-06: clearer section comments.

#ifndef SHOWCASE_PROTOCOL_H
#define SHOWCASE_PROTOCOL_H

#include <Arduino.h>
#include <cstring>

// ============================================
// SYSTEM CONFIGURATION
// ============================================
#define BROADCAST_CHANNEL 1
static const uint8_t BROADCAST_MAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#define MSG_MAX_LENGTH 64

// ============================================
// MESSAGE TYPES - Enum for type safety
// ============================================
enum MessageType : uint8_t {
    MSG_IDENTITY_ASSIGN    = 1,   // St1 Core2 -> StickC
    MSG_AUTH_REQUEST       = 2,   // St2 Matrix -> StickC
    MSG_AUTH_SUCCESS       = 3,   // St2 Echo/Matrix -> StickC
    MSG_EARN_COIN          = 4,   // St3 Paper -> StickC
    MSG_SPEND_REQUEST      = 5,   // St4 Paper -> StickC
    MSG_SPEND_CONFIRM      = 6,   // St4 Matrix -> StickC
    MSG_RESET_ALL          = 99,  // Reset Button -> All
    MSG_HEARTBEAT          = 100, // Keep-alive
    MSG_BALANCE_UPDATE     = 101, // StickC -> All (broadcast balance)
    MSG_ERROR              = 200  // Error message
};

// ============================================
// DATA STRUCTURES - Fixed 64 bytes for stability
// ============================================
typedef struct {
    uint8_t type;                  // MessageType enum
    char username[32];             // Username (max 31 chars + null)
    int32_t amount;                // Coin amount (int32 for safety)
    char description[16];          // Activity/Menu name
    uint8_t status;                // 0=pending, 1=success, 2=error
    uint16_t checksum;             // Simple CRC16
    uint32_t timestamp;            // Packet timestamp (anti-replay)
    uint8_t padding[2];            // Padding to 64 bytes
} __attribute__((packed)) ShowcaseMessage;

// ============================================
// COMMUNICATION CONSTANTS
// ============================================
#define STICKC_HTTP_PORT 8888
#define CORE2_HTTP_PORT 8888
#define MATRIX_HTTP_PORT 8888
#define PAPER_HTTP_PORT 8888

// IP addresses (192.168.4.x for AP mode)
#define STICKC_IP "192.168.4.2"
#define CORE2_IP "192.168.4.1"
#define MATRIX_IP "192.168.4.3"
#define PAPER_IP "192.168.4.4"

// ============================================
// UTILITY FUNCTIONS
// ============================================
static inline uint16_t calculateChecksum(const ShowcaseMessage &msg) {
    uint16_t crc = 0xFFFF;
    const uint8_t *data = (const uint8_t *)&msg;
    for (size_t i = 0; i < sizeof(msg) - 2; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xA001 & (-(crc & 1)));
        }
    }
    return crc;
}

static inline bool verifyChecksum(const ShowcaseMessage &msg) {
    return calculateChecksum(msg) == msg.checksum;
}

static inline void setChecksum(ShowcaseMessage &msg) {
    msg.checksum = calculateChecksum(msg);
}

// Create a properly formatted message
static inline ShowcaseMessage createMessage(uint8_t type, const char *username = "",
                                            int32_t amount = 0, const char *description = "") {
    ShowcaseMessage msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = type;
    msg.status = 0;  // pending
    msg.timestamp = millis();
    
    if (username) strncpy(msg.username, username, 31);
    if (description) strncpy(msg.description, description, 15);
    msg.amount = amount;
    
    setChecksum(msg);
    return msg;
}

#endif
