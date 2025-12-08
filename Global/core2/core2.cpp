#include <M5Unified.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <qrcode.h>
#include <HTTPClient.h>

// 1. กำหนดค่า SoftAP
const char *ssid = "Web3Showcase_AP";
const char *password = "12345678";
const char* STICKC_IP = "192.168.4.2"; 
const int STICKC_PORT = 88;

IPAddress localIP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80);
String capturedUsername = "";

// Forward Declarations: ประกาศฟังก์ชันไว้ด้านบนเพื่อแก้ปัญหา Scope
void sendUsernameToStickC(String username);
void displayQRCode(const char* data);

// โค้ด HTML สำหรับ Captive Portal
const char* CAPTIVE_HTML = R"raw(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Web3 Student Club</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
        input[type=text], button { padding: 10px; margin: 5px 0; border-radius: 5px; border: 1px solid #ccc; width: 80%; max-width: 300px; }
        button { background-color: #4CAF50; color: white; cursor: pointer; }
    </style>
</head>
<body>
    <h1>Web3 Student Club</h1>
    <form action="/submit" method="POST">
        <label for="username">Username</label><br>
        <input type="text" id="username" name="username" placeholder="Enter Username" required><br>
        <button type="submit">Submit</button>
    </form>
</body>
</html>
)raw";

// ฟังก์ชันสำหรับส่ง Username ไปยัง StickC-Plus2
void sendUsernameToStickC(String username) {
    if (WiFi.getMode() == WIFI_AP) {
        HTTPClient http;
        String url = "http://" + String(STICKC_IP) + ":" + String(STICKC_PORT) + "/set_username";

        http.begin(url);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        String postData = "username=" + username;

        int httpResponseCode = http.POST(postData);

        M5.Lcd.setCursor(5, 100);
        if (httpResponseCode > 0) {
            M5.Lcd.printf("Sent to StickC (Code: %d)", httpResponseCode);
        } else {
            M5.Lcd.printf("StickC Request Failed (Error: %s)", http.errorToString(httpResponseCode).c_str());
        }

        http.end();
    }
}

// ฟังก์ชันสำหรับแสดง QR Code
void displayQRCode(const char* data) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextDatum(top_center);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setFont(&fonts::Font4); 
    M5.Lcd.drawString("Scan Here", M5.Lcd.width() / 2, 10); 

    int size = 180;
    int x = (M5.Lcd.width() - size) / 2;
    int y = 50;
    
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, 0, data);
    M5.Lcd.qrcode(data, x, y, size);
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Lcd.setRotation(1); 

    // 1. สร้าง WiFi Access Point (SoftAP)
    M5.Lcd.print("Setting up SoftAP...");
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(localIP, gateway, subnet);
    bool result = WiFi.softAP(ssid, password);
    
    if (result) {
        M5.Lcd.printf("\nSoftAP Ready! SSID: %s\n", ssid);
        M5.Lcd.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());
    } else {
        M5.Lcd.println("SoftAP Failed!");
        return;
    }

    // 2. แสดง QR Code สำหรับการเชื่อมต่อ
    String qrData = "WIFI:S:" + String(ssid) + ";T:WPA;P:" + String(password) + ";;";
    displayQRCode(qrData.c_str());

    // 3. ตั้งค่า Web Server สำหรับ Captive Portal
    
    // A. Handle Root Path (หน้า Captive Portal หลัก)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", CAPTIVE_HTML); 
    });

    // B. Handle Submission (รับ Username)
    server.on("/submit", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("username", true)) {
            // ดึง Username จาก Form Data
            String capturedUsername = request->getParam("username", true)->value();

            M5.Lcd.fillScreen(BLACK);
            M5.Lcd.setTextDatum(top_left);
            M5.Lcd.setFont(&fonts::Font2);
            M5.Lcd.printf("Username Received: %s", capturedUsername.c_str());
            
            request->send(200, "text/plain", "Username '" + capturedUsername + "' submitted successfully.");
            
            // 7a. ส่งข้อมูล Username ที่ได้รับมาไปยัง StickC-Plus2
            sendUsernameToStickC(capturedUsername);
            
        } else {
            request->send(400, "text/plain", "Missing Username");
        }
    });

    server.begin();
    M5.Lcd.println("\nHTTP Server Started.");
}

void loop() {
    M5.update();
}