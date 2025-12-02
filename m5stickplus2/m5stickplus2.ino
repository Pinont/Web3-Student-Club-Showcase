#include <M5Unified.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// --- CENTRALIZED CONFIGURATION ---
const char *SSID_AP = "Web3Showcase_AP";
const char *PASSWORD_AP = "12345678";
const int LOCAL_PORT = 88; // Port ที่ StickC รอรับ Request
const IPAddress local_IP(192,168,4,2);
const IPAddress gateway(192,168,4,1);
const IPAddress subnet(255,255,255,0);

AsyncWebServer stickCServer(LOCAL_PORT);
String currentUsername = "Not Registered";
String authenStatus = "X";
int ccoin = 0;
String alertText = "Waiting for Identity...";

// --- UI DATA ---
// ตัวแปรสำหรับเก็บ MAC Address ของตัวเอง (ใช้ในการอ้างอิงสำหรับ Atom Matrix ใน Station 2)
String myMacAddress = "";

// --- CORE FUNCTIONS ---

// 9a. แสดงข้อมูล Username และสถานะของผู้เข้าร่วมผ่านทางหน้าจอ
void updateDisplay() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextDatum(top_left);
    M5.Lcd.setFont(&fonts::Font2);
    
    M5.Lcd.setCursor(5, 5);
    M5.Lcd.print("MAC: "); M5.Lcd.println(myMacAddress); // แสดง MAC Address เพื่อระบุตัวตน
    M5.Lcd.setCursor(5, 25);
    M5.Lcd.printf("Username: %s\n", currentUsername.c_str());
    M5.Lcd.printf("Authen Status: %s\n", authenStatus.c_str());
    M5.Lcd.printf("CCoin: %d\n", ccoin);
    
    M5.Lcd.setCursor(5, 85);
    M5.Lcd.printf("ALERT: \n%s", alertText.c_str());
}

// Handler สำหรับรับ Username จาก Core2 (/set_username)
void handleUsername(AsyncWebServerRequest *request) {
    if (request->hasParam("username", true)) {
        String receivedUsername = request->getParam("username", true)->value();

        // 8a. StickC-Plus2 ส่งเสียงแจ้งเตือนเมื่อได้รับข้อมูล
        M5.Speaker.tone(1500, 150); 
        
        // 9a. อัพเดทสถานะบนหน้าจอ
        currentUsername = receivedUsername;
        alertText = "Identity Created Successfully";
        authenStatus = "x"; 
        updateDisplay(); 
        
        request->send(200, "text/plain", "Username received.");
    } else {
        request->send(400, "text/plain", "Missing Username Parameter.");
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
    M5.Lcd.setRotation(3); 

    // Assign static IP
    WiFi.config(local_IP, gateway, subnet);

    // 1. เชื่อมต่อ Wi-Fi เข้ากับ SoftAP ของ Core2
    WiFi.begin(SSID_AP, PASSWORD_AP);
    
    while (WiFi.status() != WL_CONNECTED) { 
        delay(500);
    }
    
    // เก็บ MAC Address ของตัวเอง
    myMacAddress = WiFi.macAddress(); 
    
    M5.Lcd.printf("Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    M5.Lcd.printf("MAC: %s (Target for Station 2)\n", myMacAddress.c_str());

    // 2. ตั้งค่า Server เพื่อรอรับ Request จาก Core2
    stickCServer.on("/set_username", HTTP_POST, handleUsername);
    stickCServer.on("/add_coin", HTTP_POST, handleCoin);
    stickCServer.begin();

    // แสดง UI เริ่มต้น
    updateDisplay();
}

void loop() {
    M5.update();
}