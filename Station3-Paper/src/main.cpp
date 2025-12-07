#include <Arduino.h>
#include <M5Unified.h>
#include <esp_now.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "../include/ShowcaseProtocol.h"

// ============================================
// CONFIGURATION
// ============================================
#define PAPER_WIDTH 960
#define PAPER_HEIGHT 540
#define ACTIVITY_COUNT 4

// ============================================
// ACTIVITY DEFINITIONS
// ============================================
struct Activity {
    const char *name;
    int32_t reward;
    bool selected;
};

Activity activities[ACTIVITY_COUNT] = {
    {"Read Book", 50, false},
    {"Attend Class", 75, false},
    {"Help Friend", 100, false},
    {"Exercise", 60, false}
};

// ============================================
// GLOBAL STATE
// ============================================
struct {
    String currentUsername;
    int32_t balance;
    int selectedActivityIdx;
    bool submitting;
    uint32_t lastUpdateTime;
} g_paper = {
    "Guest",
    0,
    -1,
    false,
    0
};

// ============================================
// FORWARD DECLARATIONS
// ============================================
void submitActivity();

// ============================================
// UI DRAWING
// ============================================
void drawUI() {
    M5.Lcd.fillScreen(TFT_WHITE);

    // ===== HEADER =====
    M5.Lcd.setTextSize(3);
    M5.Lcd.setTextColor(TFT_BLACK);
    M5.Lcd.drawString("STATION 3: Earn Value", 50, 30);

    // ===== USER INFO =====
    char userBuf[128];
    snprintf(userBuf, sizeof(userBuf), "User: %s | Balance: %d coins",
             g_paper.currentUsername.c_str(), g_paper.balance);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString(userBuf, 50, 90);

    // ===== ACTIVITIES =====
    M5.Lcd.drawString("Select an activity:", 50, 150);

    int yPos = 200;
    int boxWidth = 350;
    int boxHeight = 80;
    int spacing = 20;

    for (int i = 0; i < ACTIVITY_COUNT; i++) {
        int xPos = 50 + (i % 2) * (boxWidth + spacing + 50);
        int yPosActual = (i < 2) ? 200 : (200 + boxHeight + spacing);

        // Draw box
        uint16_t boxColor = (g_paper.selectedActivityIdx == i) ? TFT_BLACK : TFT_LIGHTGREY;
        M5.Lcd.drawRect(xPos, yPosActual, boxWidth, boxHeight, boxColor);

        // Activity text
        M5.Lcd.setTextSize(2);
        M5.Lcd.setTextColor(TFT_BLACK);
        char actBuf[64];
        snprintf(actBuf, sizeof(actBuf), "%s +%d",
                 activities[i].name, activities[i].reward);
        M5.Lcd.drawString(actBuf, xPos + 20, yPosActual + 15);

        // Selected indicator
        if (g_paper.selectedActivityIdx == i) {
            M5.Lcd.drawString("✓", xPos + boxWidth - 40, yPosActual + 15);
        }
    }

    // ===== SUBMIT BUTTON =====
    int submitY = 420;
    int submitWidth = 300;
    int submitHeight = 60;
    int submitX = (PAPER_WIDTH - submitWidth) / 2;

    M5.Lcd.drawRect(submitX, submitY, submitWidth, submitHeight, TFT_BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TFT_BLACK);
    M5.Lcd.drawString("Submit", submitX + 60, submitY + 12);
}

void drawWaitingScreen() {
    M5.Lcd.fillScreen(TFT_WHITE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setTextColor(TFT_BLACK);
    M5.Lcd.drawString("Submitting...", 200, 200);
}

// ============================================
// TOUCH INPUT HANDLER (Enhanced)
// ============================================
void handleTouchInput() {
    auto t = M5.Touch.getDetail();
    if (!t.isPressed()) return;

    uint16_t touchX = t.x;
    uint16_t touchY = t.y;

    // Debounce - ignore rapid successive touches
    static uint32_t lastTouchTime = 0;
    if (millis() - lastTouchTime < 200) return;
    lastTouchTime = millis();

    // Check activity selection
    int boxWidth = 350;
    int boxHeight = 80;
    int spacing = 20;

    for (int i = 0; i < ACTIVITY_COUNT; i++) {
        int xPos = 50 + (i % 2) * (boxWidth + spacing + 50);
        int yPosActual = (i < 2) ? 200 : (200 + boxHeight + spacing);

        if (touchX >= xPos && touchX <= xPos + boxWidth &&
            touchY >= yPosActual && touchY <= yPosActual + boxHeight) {
            // Selection changed - visual feedback
            if (g_paper.selectedActivityIdx != i) {
                g_paper.selectedActivityIdx = i;
                Serial.printf("[OK] Activity selected: %s\n", activities[i].name);
                drawUI();
            }
            return;
        }
    }

    // Check submit button
    int submitY = 420;
    int submitWidth = 300;
    int submitHeight = 60;
    int submitX = (PAPER_WIDTH - submitWidth) / 2;

    if (touchX >= submitX && touchX <= submitX + submitWidth &&
        touchY >= submitY && touchY <= submitY + submitHeight) {
        if (g_paper.selectedActivityIdx >= 0 && !g_paper.submitting) {
            Serial.println("[INFO] Submit button pressed");
            submitActivity();
        }
        return;
    }
}

void submitActivity() {
    if (g_paper.selectedActivityIdx < 0 || g_paper.submitting) return;

    g_paper.submitting = true;
    drawWaitingScreen();

    Activity &selected = activities[g_paper.selectedActivityIdx];
    ShowcaseMessage msg = createMessage(MSG_EARN_COIN, g_paper.currentUsername.c_str(),
                                        selected.reward, selected.name);

    esp_err_t result = esp_now_send(BROADCAST_MAC, (uint8_t *)&msg, sizeof(msg));

    if (result == ESP_OK) {
        Serial.printf("[OK] Submitted activity: %s (+%d)\n", selected.name, selected.reward);
    } else {
        Serial.println("[ERROR] Failed to send activity");
    }

    delay(1000);
    g_paper.submitting = false;
    g_paper.selectedActivityIdx = -1;
    drawUI();
}

// ============================================
// ESP-NOW MESSAGE HANDLER
// ============================================
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    if (len != sizeof(ShowcaseMessage)) return;

    ShowcaseMessage msg;
    memcpy(&msg, incomingData, sizeof(msg));

    if (!verifyChecksum(msg)) {
        Serial.println("[WARN] Checksum verification failed");
        return;
    }

    switch (msg.type) {
        case MSG_IDENTITY_ASSIGN:
            if (msg.username[0] != '\0') {
                g_paper.currentUsername = String(msg.username);
                g_paper.balance = 0;
                g_paper.selectedActivityIdx = -1;
                Serial.printf("[OK] Identity updated: %s\n", msg.username);
                drawUI();
            }
            break;

        case MSG_EARN_COIN:
            g_paper.balance += msg.amount;
            Serial.printf("[OK] Balance updated: %d\n", g_paper.balance);
            drawUI();
            break;

        case MSG_RESET_ALL:
            g_paper.currentUsername = "Guest";
            g_paper.balance = 0;
            g_paper.selectedActivityIdx = -1;
            g_paper.submitting = false;
            Serial.println("[OK] System reset");
            drawUI();
            break;

        default:
            break;
    }
}

// ============================================
// SETUP
// ============================================
void setup() {
    M5.begin();
    Serial.begin(115200);
    delay(500);

    Serial.println("\n\n=== STATION 3: PAPER STARTING ===");

    // Clear screen
    M5.Lcd.fillScreen(TFT_WHITE);

    // Initialize WiFi
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false);
    delay(100);

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ERROR] ESP-NOW initialization failed");
        while (1) delay(1000);
    }

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, BROADCAST_MAC, 6);
    peerInfo.channel = BROADCAST_CHANNEL;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    esp_now_register_recv_cb(onDataRecv);

    Serial.println("[OK] ESP-NOW initialized");

    // Draw initial UI
    drawUI();

    Serial.println("=== STATION 3 PAPER READY ===\n");
}

// ============================================
// MAIN LOOP
// ============================================
void loop() {
    M5.update();
    handleTouchInput();

    // Periodic UI refresh
    if (millis() - g_paper.lastUpdateTime > 5000) {
        drawUI();
        g_paper.lastUpdateTime = millis();
    }

    delay(100);
}