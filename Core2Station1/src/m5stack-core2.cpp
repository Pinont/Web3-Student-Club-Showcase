#include <M5Unified.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <qrcode.h>
#include <HTTPClient.h>
#include <DNSServer.h>
#include <esp_wifi.h> 

// --- CONFIGURATION ---
const char *SSID = "Web3Showcase_AP";
const char *PASSWORD = NULL; // รหัสผ่าน

// Fixed IP Config
const IPAddress localIP(192, 168, 4, 1);
const IPAddress gateway(192, 168, 4, 1);
const IPAddress subnet(255, 255, 255, 0);

// Target StickC
const String STICKC_URL = "http://192.168.4.2:80/set_username"; 

// --- OBJECTS ---
AsyncWebServer server(80);
DNSServer dnsServer;
QRCode qrcode;

// --- STATE ---
String registeredUser = "";
bool isSuccessScreen = false;
unsigned long successTimer = 0;

// --- HTML ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Web3 Student Club</title>
  <style>
    body { font-family: sans-serif; background: #1a1a2e; color: #fff; text-align: center; padding: 20px; }
    .card { background: #16213e; padding: 30px; border-radius: 15px; box-shadow: 0 4px 15px rgba(0,0,0,0.5); }
    input { width: 100%; padding: 15px; margin: 15px 0; border-radius: 5px; border: none; font-size: 16px; }
    button { background: #4361ee; color: white; padding: 15px; border: none; border-radius: 5px; width: 100%; font-size: 18px; font-weight: bold; }
  </style>
</head>
<body>
  <div class="card">
    <h2>🎯 Station 1</h2>
    <p>Create Identity</p>
    <form action="/register" method="POST">
      <input type="text" name="username" placeholder="Enter Username" required>
      <button type="submit">REGISTER</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

// --- UI FUNCTIONS ---
void drawQRCode() {
    isSuccessScreen = false;
    M5.Display.fillScreen(BLACK);
    
    String qrData = "WIFI:S:" + String(SSID) + ";T:WPA;P:" + String(PASSWORD) + ";;";
    
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, 0, qrData.c_str());
    
    int scale = 4;
    int offsetX = (320 - (qrcode.size * scale)) / 2;
    int offsetY = (240 - (qrcode.size * scale)) / 2 - 20;

    M5.Display.fillRect(offsetX - 5, offsetY - 5, (qrcode.size * scale) + 10, (qrcode.size * scale) + 10, WHITE);

    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                M5.Display.fillRect(x * scale + offsetX, y * scale + offsetY, scale, scale, BLACK);
            }
        }
    }
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(CYAN);
    M5.Display.drawCenterString("Scan to Register", 160, 215);
}

void drawSuccess(String user) {
    isSuccessScreen = true;
    successTimer = millis();
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextColor(GREEN);
    M5.Display.setTextSize(3);
    M5.Display.drawCenterString("SUCCESS!", 160, 60);
    M5.Display.setTextColor(YELLOW);
    M5.Display.drawCenterString(user, 160, 140);
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(2);
    M5.Display.drawCenterString("Check StickC", 160, 200);
}

bool sendToStickC(String user) {
    HTTPClient http;
    http.begin(STICKC_URL);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.setConnectTimeout(2000); 
    String postData = "username=" + user;
    int httpCode = http.POST(postData);
    http.end();
    return (httpCode == 200);
}

// --- SETUP ---
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(1);
    Serial.begin(115200);
    
    // ----------------------------------------------------
    // 🛠️ 1. ANDROID CONNECTION FIX (ลำดับสำคัญมาก)
    // ----------------------------------------------------
    WiFi.mode(WIFI_AP);
    
    // สั่งปิด WiFi ก่อนเพื่อแก้ Config
    esp_wifi_stop();
    esp_wifi_deinit();
    
    // โหลด Config ใหม่และปิด AMPDU RX (ตัวปัญหาของ Android)
    wifi_init_config_t my_config = WIFI_INIT_CONFIG_DEFAULT();
    my_config.ampdu_rx_enable = false; 
    esp_wifi_init(&my_config);
    esp_wifi_start();
    delay(100);

    // เริ่มสร้าง AP *หลังจาก* แก้ Config แล้วเท่านั้น
    WiFi.softAPConfig(localIP, gateway, subnet);
    WiFi.softAP(SSID, PASSWORD);
    
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
    // ----------------------------------------------------

    // 2. DNS Server (Captive Portal หัวใจหลัก)
    // ดักจับทุก Domain (*) ให้ชี้มาที่ IP เรา
    dnsServer.setTTL(300);
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", localIP);

    // 3. Routes for Captive Portal (ดักทุกทาง)
    // Android Check
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *r){ r->redirect("http://192.168.4.1/"); });
    server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest *r){ r->redirect("http://192.168.4.1/"); });
    // Windows Check
    server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *r){ r->redirect("http://192.168.4.1/"); });
    server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *r){ r->redirect("http://logout.net"); });
    // Apple Check
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *r){ r->redirect("http://192.168.4.1/"); });
    
    // Catch-All: ถ้าหาอะไรไม่เจอ ให้เด้งเข้าหน้าแรกเราให้หมด
    server.onNotFound([](AsyncWebServerRequest *request){
        request->redirect("http://192.168.4.1/");
    });

    // Main Page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });

    // Register Handler
    server.on("/register", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("username", true)) {
            String user = request->getParam("username", true)->value();
            bool success = sendToStickC(user);
            
            String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'><style>body{background:#1a1a2e;color:white;text-align:center;font-family:sans-serif;padding:50px;}</style></head><body>";
            if(success) {
                html += "<h1 style='color:#00ff00;font-size:60px'>✅</h1><h2>Success!</h2><p>ID: <b>" + user + "</b></p>";
                drawSuccess(user);
            } else {
                html += "<h1 style='color:red;font-size:60px'>⚠️</h1><h2>Connection Error</h2><p>StickC not found</p><button onclick='history.back()'>Retry</button>";
            }
            html += "</body></html>";
            request->send(200, "text/html", html);
        } else {
            request->send(400, "text/plain", "Missing Username");
        }
    });

    server.begin();
    drawQRCode();
}

void loop() {
    M5.update();
    // สำคัญ: ต้องเรียกตลอดเวลาเพื่อให้มือถือรู้ว่าต้องเด้งหน้าเว็บ
    dnsServer.processNextRequest();

    // Reset หน้าจอกลับเป็น QR Code หลังจาก 5 วินาที
    if (isSuccessScreen && (millis() - successTimer > 5000)) {
        drawQRCode();
    }
}
