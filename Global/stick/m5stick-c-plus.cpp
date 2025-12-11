#include <M5Unified.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <esp_gap_ble_api.h> // เพิ่ม Library นี้สำหรับเปลี่ยนชื่อแบบ Fast Mode

// --- CONFIGURATION ---
const char *SSID_AP = "Web3Showcase_AP";
const char *PASSWORD_AP = "12345678";
const int LOCAL_PORT = 88; 

// --- FIX IP SETTINGS ---
IPAddress localIP(192, 168, 4, 10);
IPAddress gateway(192, 168, 4, 1);   
IPAddress subnet(255, 255, 255, 0); 

// --- GLOBALS ---
AsyncWebServer stickCServer(LOCAL_PORT);
String currentUsername = "Not Registered";
String authenStatus = "X";
int ccoin = 0;
String alertText = "Waiting for Identity...";
String myMacAddress = "";

BLEServer *pServer = NULL;
BLEAdvertising *pAdvertising = NULL;

// Flag ฝากงานให้ Loop ทำ
bool shouldRestartBLE = false;
String newBLEName = "";

// --- FUNCTIONS ---

// 1. ฟังก์ชันเปลี่ยนชื่อ BLE (ฉบับแก้ไข: ไม่ต้องปิดระบบ Matrix เห็นไวมาก)
void startBLE(String name) {
    // กรณีเพิ่งเริ่มครั้งแรก (Start)
    if (pAdvertising == NULL) {
        BLEDevice::init(name.c_str()); 
        pServer = BLEDevice::createServer();
        pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);  
        pAdvertising->setMinPreferred(0x12);
    } else {
        // กรณีเปลี่ยนชื่อ: แค่หยุดประกาศชั่วคราว (Stop)
        pAdvertising->stop();
    }

    // --- ส่วนสำคัญ: เปลี่ยนชื่อระดับ Hardware โดยไม่ต้อง deinit ---
    esp_ble_gap_set_device_name(name.c_str());
    // -------------------------------------------------------

    // จัดกระเป๋าใบที่ 1 (Main): ใส่ UUID 1234
    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
    oAdvertisementData.setFlags(0x06);
    oAdvertisementData.setCompleteServices(BLEUUID("1234")); 
    pAdvertising->setAdvertisementData(oAdvertisementData);

    // จัดกระเป๋าใบที่ 2 (Response): ใส่ชื่อใหม่
    BLEAdvertisementData oScanResponseData = BLEAdvertisementData();
    oScanResponseData.setName(name.c_str()); 
    pAdvertising->setScanResponseData(oScanResponseData);

    // เริ่มประกาศอีกครั้ง (Start)
    pAdvertising->start();
    
    Serial.printf("[BLE] Name Updated to: %s (UUID: 1234)\n", name.c_str());
}

// 2. ฟังก์ชันอัปเดตหน้าจอ
void updateDisplay() {
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextDatum(top_left);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.setTextColor(WHITE);
    
    M5.Display.setCursor(5, 5);
    M5.Display.printf("IP: %s\n", WiFi.localIP().toString().c_str()); 
    M5.Display.setCursor(5, 25);
    M5.Display.printf("User: %s\n", currentUsername.c_str());
    M5.Display.printf("Status: %s\n", authenStatus.c_str());
    M5.Display.printf("CCoin: %d\n", ccoin);
    
    if (authenStatus == "/") M5.Display.setTextColor(GREEN);
    else M5.Display.setTextColor(ORANGE);
    
    M5.Display.setCursor(5, 95);
    M5.Display.printf("MSG: %s", alertText.c_str());
}

// 3. Handler รับ Username จาก Core2
void handleUsername(AsyncWebServerRequest *request) {
    String receivedUsername = "";
    if (request->hasParam("username", true)) {
        receivedUsername = request->getParam("username", true)->value();
    } else if (request->hasParam("username", false)) {
        receivedUsername = request->getParam("username", false)->value();
    }

    if (receivedUsername != "") {
        M5.Speaker.tone(1500, 150); 
        
        Serial.printf("[HTTP] Received Username: %s\n", receivedUsername.c_str());

        currentUsername = receivedUsername;
        alertText = "Identity Confirmed!";
        authenStatus = "/"; 
        
        // ยกธงบอก Loop ให้เปลี่ยนชื่อ
        newBLEName = receivedUsername;
        shouldRestartBLE = true; 
        
        updateDisplay(); 
        request->send(200, "text/plain", "OK");
    } else {
        request->send(400, "text/plain", "Fail: Missing Username");
    }
}

// Handler สำหรับรับ Coin จาก Paper (/add_coin)
void handleCoin(AsyncWebServerRequest *request) {
    if (request->hasParam("value", true)) {
        String coin = request->getParam("value", true)->value();

        M5.Speaker.tone(1500, 150);

        ccoin += coin.toInt();
        alertText = "Receive " + coin + " CCoin";
        updateDisplay();

        request->send(200, "text/plain", "Coin received.");
    } else {
        request->send(400, "text/plain", "Missing value Parameter.");
    }
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(3); 
    Serial.begin(115200);

    // --- Phase 1: Connecting WiFi ---
    M5.Display.fillScreen(BLACK);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextDatum(middle_center);
    M5.Display.drawString("Connecting WiFi...", M5.Display.width()/2, M5.Display.height()/2 - 10);

    WiFi.mode(WIFI_STA);
    
    WiFi.config(localIP, gateway, subnet);
    WiFi.begin(SSID_AP, PASSWORD_AP);
    
    Serial.println("[WiFi] Connecting...");
    
    M5.Display.setTextDatum(top_left);
    M5.Display.setCursor(10, M5.Display.height()/2 + 10);
    while (WiFi.status() != WL_CONNECTED) { 
        delay(500);
        M5.Display.print("."); 
        Serial.print(".");
    }
    Serial.println("\n[WiFi] Connected!");
    
    // --- Phase 2: System Ready ---
    myMacAddress = WiFi.macAddress();
    
    stickCServer.on("/set_username", HTTP_POST, handleUsername);
    stickCServer.on("/set_username", HTTP_GET, handleUsername); 
    stickCServer.on("/add_coin", HTTP_POST, handleCoin);
    stickCServer.begin();

    // เริ่ม BLE ครั้งแรก
    startBLE("GUEST-PLAYER");

    updateDisplay();
}

void loop() {
    // --- โซนทำงานหนัก (ปลอดภัย ไม่รีเซ็ต) ---
    if (shouldRestartBLE) {
        delay(100);
        M5.Speaker.tone(2000, 300);

        // เรียกฟังก์ชันเปลี่ยนชื่อแบบใหม่ (ที่ไม่พัง)
        startBLE(newBLEName);
        
        shouldRestartBLE = false; 
    }
    
    M5.update();
}