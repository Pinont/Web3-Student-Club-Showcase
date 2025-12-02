#include <M5Unified.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <qrcode.h>
#include <HTTPClient.h>
#include <DNSServer.h>

// --- CONFIGURATION ---
const char *SSID = "Web3Showcase_AP";
const char *PASSWORD = "12345678";
const char* STICKC_IP = "192.168.4.20"; 
const int STICKC_PORT = 88;

IPAddress localIP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// สร้างตัวแปร String สำหรับ URL ของ IP เพื่อใช้ในโค้ดใหม่
String localIPURL = "http://192.168.4.1";

AsyncWebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

String penningUsername = "";
bool hasNewUserName = false;

// --- HTML (เปลี่ยนชื่อตัวแปรเป็น index_html ตามโค้ดใหม่) ---
const char* index_html = R"raw(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Web3 Student Club Identity</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
        input[type=text], button { padding: 10px; margin: 5px 0; border-radius: 5px; border: 1px solid #ccc; width: 80%; max-width: 300px; }
        button { background-color: #4CAF50; color: white; cursor: pointer; }
    </style>
</head>
<body>
    <h1>STATION 1: Identity Creation</h1>
    <form action="/submit" method="POST">
        <label for="username">Enter Your Username:</label><br>
        <input type="text" id="username" name="username" placeholder="Username (e.g. Satoshi_N)" required><br>
        <button type="submit">Submit</button>
    </form>
</body>
</html>
)raw";

// --- CORE FUNCTIONS ---

void sendUsernameToStickC(String username) {
    HTTPClient http;
    String url = "http://" + String(STICKC_IP) + ":" + String(STICKC_PORT) + "/set_username"; 

    http.begin(url);
    http.setConnectTimeout(2000); // Timeout 2 วินาที กันค้าง
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String postData = "username=" + username;

    int httpResponseCode = http.POST(postData);

    M5.Lcd.setCursor(5, 100);
    if (httpResponseCode > 0) {
        M5.Lcd.printf("Sent to StickC (Code: %d) ", httpResponseCode);
    } else {
        M5.Lcd.printf("StickC Request Failed (Error: %s)", http.errorToString(httpResponseCode).c_str());
    }
    http.end();
}

void displayQRCode(const char* data) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextDatum(top_center);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setFont(&fonts::Font4); 
    M5.Lcd.drawString("1. Connect to Wi-Fi", M5.Lcd.width() / 2, 10); 

    int size = 180;
    int x = (M5.Lcd.width() - size) / 2;
    int y = 50;
    
    M5.Lcd.qrcode(data, x, y, size);
    
    M5.Lcd.setTextDatum(top_left);
    M5.Lcd.setCursor(5, 240);
    M5.Lcd.setFont(&fonts::Font2);
    M5.Lcd.print("2. Enter Username in Browser");
}

// --- SETUP SERVER FUNCTION (รวมโค้ดใหม่ที่นี่) ---
void setUpWebserver() {
    // 1. Required Handlers (Windows 11 & PAD Fixes)
    server.on("/connecttest.txt", [](AsyncWebServerRequest *request) { request->redirect("http://logout.net"); }); 
    server.on("/wpad.dat", [](AsyncWebServerRequest *request) { request->send(404); }); 

    // 2. Android Specific (แก้ตรงนี้: ส่งหน้าเว็บ index_html สวนกลับไปเลย ไม่ต้อง redirect)
    // Android รุ่นใหม่
    server.on("/generate_204", [](AsyncWebServerRequest *request) { 
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", index_html);
        response->addHeader("Cache-Control", "public,max-age=31536000");
        request->send(response);
    });
    // Android รุ่นเก่า
    server.on("/gen_204", [](AsyncWebServerRequest *request) { 
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", index_html);
        response->addHeader("Cache-Control", "public,max-age=31536000");
        request->send(response);
    });

    // 3. Background responses (Microsoft, Apple, Firefox) - พวกนี้ Redirect เหมือนเดิมดีแล้ว
    server.on("/redirect", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });          // Microsoft
    server.on("/hotspot-detect.html", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); }); // Apple
    server.on("/canonical.html", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });    // Firefox
    server.on("/success.txt", [](AsyncWebServerRequest *request) { request->send(200); });                  // Firefox
    server.on("/ncsi.txt", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });          // Windows

    // 4. Favicon 
    server.on("/favicon.ico", [](AsyncWebServerRequest *request) { request->send(404); });

    // 5. Serve Basic HTML Page (Root)
    server.on("/", HTTP_ANY, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", index_html);
        response->addHeader("Cache-Control", "public,max-age=31536000");
        request->send(response);
        Serial.println("Served Basic HTML Page");
    });

    // 6. Handle Form Submission
    server.on("/submit", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("username", true)) {
            String username = request->getParam("username", true)->value();
            penningUsername = username;
            hasNewUserName = true;
            M5.Lcd.fillScreen(BLACK);
            M5.Lcd.setTextDatum(top_left);
            M5.Lcd.setFont(&fonts::Font2);
            M5.Lcd.printf("Username: %s", username.c_str());
            request->send(200, "text/plain", "Success! Check the StickC.");
        } else {
            request->send(400, "text/plain", "Missing Username");
        }
    });

    // 7. Catch All (onNotFound) - ส่งหน้าเว็บสวนไปเลยเช่นกัน
    server.onNotFound([](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", index_html);
        request->send(response); // ไม่ Redirect แล้ว ส่งหน้าเว็บใส่เลย
        Serial.print("Caught: ");
        Serial.println(request->url());
    });

    server.begin();
    M5.Lcd.println("\nHTTP Server Started.");
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Lcd.setRotation(1); 
    Serial.begin(115200); // เริ่มต้น Serial เพื่อดู Debug จากโค้ดใหม่

    // WiFi Setup
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(localIP, gateway, subnet);
    WiFi.softAP(SSID, PASSWORD);
    
    // Display Info
    M5.Lcd.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    M5.Lcd.printf("AP MAC: %s\n", WiFi.softAPmacAddress().c_str());

    // QR Code
    String qrData = "WIFI:S:" + String(SSID) + ";T:WPA;P:" + String(PASSWORD) + ";;";
    displayQRCode(qrData.c_str());

    // DNS Server
    dnsServer.start(DNS_PORT, "*", localIP);

    // เริ่มต้น Webserver ด้วยฟังก์ชันใหม่
    setUpWebserver();
}

void loop() {
    dnsServer.processNextRequest();

    if(hasNewUserName){
        sendUsernameToStickC(penningUsername);
        hasNewUserName = false;
        // หลังจากส่งเสร็จ อาจจะให้กลับมาหน้า QR Code หรือหน้า Info ก็ได้
        delay(2000);
        String qrData = "WIFI:S:" + String(SSID) + ";T:WPA;P:" + String(PASSWORD) + ";;";
        displayQRCode(qrData.c_str());
    }
    M5.update();
}