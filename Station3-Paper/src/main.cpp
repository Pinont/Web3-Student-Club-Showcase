// Station3_Paper.cpp: โค้ดสำหรับ M5-Paper (Activity Dashboard/Earn Value)
// จัดการ E-Ink Display, Touch Input, และส่ง Request Earn CCoin

#include <M5EPD.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include "config.h"

AsyncWebServer server(80);

// ข้อมูลกิจกรรม
struct Activity {
    String name;
    int coin;
};

Activity activityList[] = {
    {"Walk 1000 steps", 10},
    {"Recycle bottle", 5},
    {"Bike 1 km", 20},
    {"Reuse cup", 5}
};
const int NUM_ACTIVITIES = 4;

int selectedIndex = -1;
String acknowledgeText = "";

// ตำแหน่งของปุ่มและข้อความ (ค่าประมาณ)
const int START_Y = 100;
const int ROW_HEIGHT = 50;
const int BUTTON_HEIGHT = 40;
const int BUTTON_Y = START_Y + NUM_ACTIVITIES * ROW_HEIGHT + 30;

// --- ฟังก์ชันช่วยเหลือด้าน HTTP Client ---
void sendPostRequest(IPAddress ip, const char* endpoint, const String& body) {
    HTTPClient http;
    String url = String("http://") + ip.toString() + endpoint;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.POST(body);
    http.end();
}

// --- ฟังก์ชันช่วยเหลือด้าน UI (E-Ink) ---
void drawActivityDashboard(m5epd_update_mode_t mode = UPDATE_MODE_GC16) {
    M5.EPD.WriteFullWindow(M5EPD_RECT_WINDOW_W, M5EPD_RECT_WINDOW_H, mode); // clear display

    M5.EPD.SetFont(&AsciiFont8);
    M5.EPD.SetTextColor(0x0000, 0xFFFF); // Black text on White background

    // Title
    M5.EPD.DrawString("3. Earn Value from Actions", 50, 20);
    M5.EPD.DrawString("What did you do today?", 50, 60);

    // Activity List
    for (int i = 0; i < NUM_ACTIVITIES; i++) {
        String text = String(i + 1) + ". " + activityList[i].name + " -> " + String(activityList[i].coin) + " CCoin";
        
        // Highlight selection
        if (i == selectedIndex) {
            M5.EPD.FillRect(50, START_Y + i * ROW_HEIGHT, 540, ROW_HEIGHT, 0x0000); // Black background
            M5.EPD.SetTextColor(0xFFFF, 0x0000); // White text
        } else {
            M5.EPD.FillRect(50, START_Y + i * ROW_HEIGHT, 540, ROW_HEIGHT, 0xFFFF); // White background
            M5.EPD.SetTextColor(0x0000, 0xFFFF); // Black text
        }
        
        M5.EPD.DrawString(text.c_str(), 60, START_Y + i * ROW_HEIGHT + (ROW_HEIGHT / 2) - 10);
    }
    
    // Draw Submit Button
    M5.EPD.FillRect(200, BUTTON_Y, 240, BUTTON_HEIGHT, 0x0000);
    M5.EPD.SetTextColor(0xFFFF, 0x0000);
    M5.EPD.DrawString("SUBMIT", 230, BUTTON_Y + (BUTTON_HEIGHT / 2) - 10);

    // Acknowledgement Text
    M5.EPD.SetTextColor(0x0000, 0xFFFF);
    M5.EPD.FillRect(50, BUTTON_Y + BUTTON_HEIGHT + 40, 540, 50, 0xFFFF);
    M5.EPD.DrawString(acknowledgeText.c_str(), 60, BUTTON_Y + BUTTON_HEIGHT + 50);

    M5.EPD.UpdateFull(mode);
}

// --- ฟังก์ชัน Touch Handler ---
void handleTouch(int x, int y) {
    // Check Activity List Area
    for (int i = 0; i < NUM_ACTIVITIES; i++) {
        if (x > 50 && x < 590 && y > START_Y + i * ROW_HEIGHT && y < START_Y + (i + 1) * ROW_HEIGHT) {
            selectedIndex = i;
            acknowledgeText = "Selected: " + activityList[i].name + " (+ " + String(activityList[i].coin) + " CCoin)";
            drawActivityDashboard(UPDATE_MODE_DU4);
            return;
        }
    }

    // Check Submit Button Area
    if (x > 200 && x < 440 && y > BUTTON_Y && y < BUTTON_Y + BUTTON_HEIGHT) {
        if (selectedIndex != -1) {
            // 20. ส่ง Request พร้อมข้อมูล CCoin ไปยัง StickC-Plus2
            Activity selected = activityList[selectedIndex];
            String body = "{\"amount\": " + String(selected.coin) + "}";
            
            sendPostRequest(IP_STICKC, ENDPOINT_EARN_COIN, body);
            
            // 19. Update Text แสดงกิจกรรมที่เลือกและ CCoin ที่ได้รับ
            acknowledgeText = "Submitted! You'll receive: " + String(selected.coin) + " CCoin for " + selected.name;
            selectedIndex = -1; // Clear selection after submission
            drawActivityDashboard(UPDATE_MODE_DU4);
        } else {
            acknowledgeText = "Please select an activity first.";
            drawActivityDashboard(UPDATE_MODE_DU4);
        }
    }
}

// --- ฟังก์ชัน HTTP Server Handlers ---
void handleSystemReset(AsyncWebServerRequest *request) {
    selectedIndex = -1;
    acknowledgeText = "";
    // 52. รีเซ็ตการเลือกกิจกรรม/เมนู
    drawActivityDashboard();
    request->send(200, "text/plain", "M5-Paper S3 reset complete.");
}

// --- Setup Function ---
void setup() {
    M5.begin();
    M5.EPD.SetRotation(90); // Landscape
    Serial.begin(115200);

    // กำหนด Static IP
    WiFi.config(IP_PAPER_S3, IP_CORE2, IPAddress(255, 255, 255, 0));
    WiFi.begin(AP_SSID, AP_PASSWORD);

    Serial.println("Connecting to AP...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.print("\nConnected to AP. IP: ");
    Serial.println(WiFi.localIP());

    // ตั้งค่า Server Endpoints
    server.on(ENDPOINT_RESET_MENU, HTTP_POST, handleSystemReset);

    server.begin();

    // 17. แสดง Activity List
    drawActivityDashboard();
}

void loop() {
    M5.update();
    
    // Handle Touch Input
    if (M5.TP.avaliable()) {
        M5.TP.Get>>tpData;
        if (tpData.touches != 0) {
            // Assume single touch for simplicity
            handleTouch(tpData.points[0].x, tpData.points[0].y);
        }
        tpData.touches = 0;
    }
    
    delay(50);
}