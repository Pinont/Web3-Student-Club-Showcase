#include <M5Unified.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// --- CENTRALIZED CONFIGURATION (ย้ายมาไว้ที่นี่เพื่อแก้ปัญหาตัวแปรหาย) ---
const char *WIFI_SSID = "Web3Showcase_AP";
const char *WIFI_PASS = NULL; // รหัสผ่าน

// IP Configuration
const IPAddress IP_CORE2(192, 168, 4, 1);   // Gateway
const IPAddress IP_STICKC(192, 168, 4, 2);  // This Device (StickC)
const IPAddress SUBNET(255, 255, 255, 0); 

// BLE Configuration
const char* BLE_DEVICE_NAME = "M5_Showcase_User";
const char* BLE_SERVICE_UUID = "1234";     

const int LOCAL_PORT = 80;

// --- GLOBALS ---
AsyncWebServer server(LOCAL_PORT);

// State Variables
String username = "NOT SET";
int ccoin = 0;
String auth_status = "X";
String alert_msg = "Waiting...";
bool needUpdate = true;

// --- UI FUNCTIONS ---
void drawScreen() {
    M5.Display.startWrite();
    M5.Display.fillScreen(BLACK);
    
    // Header
    M5.Display.fillRect(0, 0, 240, 30, 0x10A2); // Dark Blue
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(1.5);
    M5.Display.setCursor(5, 7);
    M5.Display.print("WEB3 ID: WALLET");

    // Username
    M5.Display.setCursor(10, 45);
    M5.Display.setTextColor(LIGHTGREY);
    M5.Display.setTextSize(1);
    M5.Display.print("USERNAME:");
    
    M5.Display.setCursor(10, 60);
    M5.Display.setTextColor(CYAN);
    M5.Display.setTextSize(2);
    M5.Display.print(username);

    // Coin & Status
    M5.Display.setCursor(10, 95);
    M5.Display.setTextColor(YELLOW);
    M5.Display.setTextSize(1.5);
    M5.Display.printf("COIN: %d", ccoin);

    M5.Display.setCursor(120, 95);
    if (auth_status == "X") M5.Display.setTextColor(RED);
    else M5.Display.setTextColor(GREEN);
    M5.Display.printf("AUTH: %s", auth_status.c_str());

    // Footer Alert
    M5.Display.fillRect(0, 115, 240, 20, 0x2104); // Dark Grey
    M5.Display.setCursor(5, 118);
    M5.Display.setTextColor(ORANGE);
    M5.Display.setTextSize(1);
    M5.Display.print(alert_msg);

    M5.Display.endWrite();
    needUpdate = false;
}

// --- WIFI FUNCTIONS ---
void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;

    M5.Display.fillScreen(BLACK);
    M5.Display.setCursor(10, 60);
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(1.5);
    M5.Display.print("Connecting to Core2...");
    
    // Config Static IP
    WiFi.config(IP_STICKC, IP_CORE2, SUBNET, IP_CORE2);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 20) {
        delay(500);
        M5.Display.print(".");
        attempt++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        needUpdate = true;
    } else {
        M5.Display.fillScreen(RED);
        M5.Display.setCursor(10, 60);
        M5.Display.print("WiFi Failed! Retrying...");
        delay(1000);
    }
}

// --- SETUP ---
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(3);

    // 1. Connect WiFi
    connectWiFi();

    // 2. Setup BLE
    BLEDevice::init(BLE_DEVICE_NAME);
    BLEServer *pServer = BLEDevice::createServer();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID); // ใช้งานตัวแปรที่ประกาศไว้ข้างบน
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); 
    pAdvertising->start();

    // 3. API Routes
    
    // Station 1: Set User
    server.on("/set_user", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasArg("name")) {
            username = request->arg("name");
            ccoin = 0;
            auth_status = "X";
            alert_msg = "Identity Created!";
            M5.Speaker.tone(1000, 100); delay(150);
            M5.Speaker.tone(2000, 300);
            needUpdate = true;
            request->send(200, "text/plain", "OK");
        } else request->send(400);
    });

    // Station 2: Auth
    server.on("/auth_start", HTTP_GET, [](AsyncWebServerRequest *request){
        alert_msg = "AUTH... In Progress";
        M5.Speaker.tone(1500, 500);
        needUpdate = true;
        request->send(200, "text/plain", "Started");
    });

    server.on("/auth_success", HTTP_GET, [](AsyncWebServerRequest *request){
        auth_status = "/";
        alert_msg = "Auth Successful!";
        M5.Speaker.tone(2000, 100); delay(100);
        M5.Speaker.tone(3000, 300);
        needUpdate = true;
        request->send(200, "text/plain", "Verified");
    });

    // Station 3: Earn
    server.on("/earn", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasArg("amount")) {
            int amt = request->arg("amount").toInt();
            ccoin += amt;
            alert_msg = "Received " + String(amt) + " Coin";
            M5.Speaker.tone(4000, 100); delay(50);
            M5.Speaker.tone(5000, 100);
            needUpdate = true;
            request->send(200, "text/plain", "OK");
        } else request->send(400);
    });

    // Station 4: Spend
    server.on("/spend", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasArg("amount")) {
            int amt = request->arg("amount").toInt();
            if(ccoin >= amt) {
                ccoin -= amt;
                alert_msg = "Spent " + String(amt) + " Coin";
                M5.Speaker.tone(1500, 300);
                needUpdate = true;
                request->send(200, "text/plain", "OK");
            } else {
                alert_msg = "Not Enough Coin!";
                M5.Speaker.tone(200, 500);
                needUpdate = true;
                request->send(402, "text/plain", "Fail");
            }
        } else request->send(400);
    });

    // Reset
    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
        username = "NOT SET";
        ccoin = 0;
        auth_status = "X";
        alert_msg = "System Reset";
        M5.Speaker.tone(500, 500);
        needUpdate = true;
        request->send(200, "text/plain", "OK");
    });

    server.begin();
    needUpdate = true;
}

void loop() {
    M5.update();
    
    // Check WiFi
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }

    if (needUpdate) {
        drawScreen();
    }
}
