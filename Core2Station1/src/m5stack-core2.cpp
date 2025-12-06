#include <M5Core2.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <esp_now.h>
#include <SPIFFS.h>
#include "../include/ShowcaseProtocol.h"

// ============================================
// CONFIGURATION
// ============================================
const char *SSID_AP = "Web3_Showcase";
const char *PASS_AP = "";
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);

// ============================================
// GLOBAL STATE
// ============================================
DNSServer dnsServer;
WebServer webServer(80);
String lastUsername = "Guest";
unsigned long lastUIUpdate = 0;
bool espNowReady = false;

// ============================================
// HTML FORM (Dark Theme + Responsive UI)
// ============================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Web3 Showcase - Identity</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
      font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, sans-serif;
      background: linear-gradient(135deg, #1a1a1a 0%, #2d2d2d 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .container { 
      background: #2d2d2d;
      border-radius: 15px;
      padding: 40px;
      box-shadow: 0 10px 40px rgba(0,0,0,0.5);
      max-width: 500px;
      width: 100%;
      border: 1px solid #444;
    }
    h1 { 
      color: #007bff;
      margin-bottom: 10px;
      font-size: 28px;
      text-align: center;
    }
    .subtitle { 
      color: #aaa;
      text-align: center;
      margin-bottom: 30px;
      font-size: 14px;
    }
    .qr-section { 
      background: #1a1a1a;
      padding: 20px;
      border-radius: 10px;
      margin-bottom: 30px;
      text-align: center;
      border: 1px solid #444;
    }
    .qr-label { 
      color: #aaa;
      font-size: 12px;
      margin-bottom: 10px;
    }
    input { 
      width: 100%;
      padding: 14px;
      margin: 15px 0;
      border: 2px solid #444;
      border-radius: 8px;
      font-size: 16px;
      background: #1a1a1a;
      color: white;
      transition: border-color 0.3s;
    }
    input:focus { 
      outline: none;
      border-color: #007bff;
    }
    input::placeholder {
      color: #666;
    }
    button { 
      width: 100%;
      padding: 14px;
      background: linear-gradient(135deg, #007bff 0%, #0056b3 100%);
      color: white;
      border: none;
      border-radius: 8px;
      font-size: 16px;
      font-weight: 600;
      cursor: pointer;
      transition: transform 0.2s, box-shadow 0.2s;
    }
    button:hover { 
      transform: translateY(-2px);
      box-shadow: 0 10px 25px rgba(0, 123, 255, 0.4);
    }
    button:active { 
      transform: translateY(0);
    }
    .info {
      color: #888;
      font-size: 12px;
      margin-top: 20px;
      text-align: center;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🎯 Web3 Showcase</h1>
    <p class="subtitle">Create Your Digital Identity</p>
    
    <div class="qr-section">
      <p class="qr-label">📱 Scan QR Code to Connect</p>
      <p style="font-size: 11px; color: #666;">WiFi: Web3_Showcase (Open Network)</p>
    </div>

    <form action="/submit" method="POST">
      <input 
        type="text" 
        name="username" 
        placeholder="Enter your name (2-12 chars)"
        minlength="2"
        maxlength="12"
        required
        autocomplete="off"
      >
      <button type="submit">✓ Create Identity</button>
    </form>

    <div class="info">
      ✓ Check your wearable device after submission
    </div>
  </div>
</body>
</html>
)rawliteral";

// ============================================
// ESP-NOW INITIALIZATION
// ============================================
void initESPNow() {
    WiFi.mode(WIFI_AP_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ERROR] ESP-NOW Init Failed");
        return;
    }

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, BROADCAST_MAC, 6);
    peerInfo.channel = BROADCAST_CHANNEL;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("[ERROR] Failed to add broadcast peer");
        return;
    }
    espNowReady = true;
    Serial.println("[OK] ESP-NOW initialized");
}

// ============================================
// SEND USERNAME TO STICKC
// ============================================
void sendIdentityToStickC(const String &username) {
    if (!espNowReady) {
        M5.Lcd.fillRect(0, 140, 320, 100, TFT_RED);
        M5.Lcd.setTextColor(TFT_WHITE);
        M5.Lcd.setCursor(10, 160);
        M5.Lcd.setTextSize(2);
        M5.Lcd.print("ESP-NOW Error");
        return;
    }

    ShowcaseMessage msg = createMessage(MSG_IDENTITY_ASSIGN, username.c_str(), 0, "");
    esp_err_t result = esp_now_send(BROADCAST_MAC, (uint8_t *)&msg, sizeof(msg));

    // Clear previous status
    M5.Lcd.fillRect(0, 140, 320, 100, TFT_BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 160);

    if (result == ESP_OK) {
        M5.Lcd.setTextColor(TFT_GREEN);
        M5.Lcd.printf("✓ Sent: %s", username.c_str());
        lastUsername = username;
        Serial.printf("[OK] Identity sent: %s\n", username.c_str());
    } else {
        M5.Lcd.setTextColor(TFT_RED);
        M5.Lcd.print("✗ Send Failed");
        Serial.println("[ERROR] Failed to send identity");
    }
    lastUIUpdate = millis();
}

// ============================================
// UI DRAWING
// ============================================
void drawUI() {
    M5.Lcd.fillScreen(TFT_BLACK);

    // Header
    M5.Lcd.fillRect(0, 0, 320, 60, TFT_NAVY);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.drawString("STATION 1", 20, 15);

    // QR Code area
    M5.Lcd.setTextColor(TFT_CYAN);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString("QR Code:", 20, 75);

    // Generate and display QR
    String qrData = "WIFI:S:" + String(SSID_AP) + ";T:nopass;;";
    M5.Lcd.qrcode(qrData.c_str(), 50, 100, 140, 5);

    // Info
    M5.Lcd.setTextColor(TFT_ORANGE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString("Go to: Web3_Showcase", 20, 320);

    // Status
    M5.Lcd.setTextColor(TFT_LIGHTGREY);
    M5.Lcd.setTextSize(1);
    M5.Lcd.drawString("Last user: " + lastUsername, 20, 350);
}

// ============================================
// SETUP
// ============================================
void setup() {
    M5.begin(true, true, true, true);
    Serial.begin(115200);
    delay(500);

    Serial.println("\n\n=== STATION 1: CORE2 STARTING ===");

    // Initialize SPIFFS if needed for static files
    if (!SPIFFS.begin(true)) {
        Serial.println("[WARN] SPIFFS mount failed");
    }

    // Draw initial UI
    drawUI();

    // Setup WiFi AP
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    if (!WiFi.softAP(SSID_AP, PASS_AP, BROADCAST_CHANNEL, 0, 4)) {
        Serial.println("[ERROR] SoftAP creation failed");
        return;
    }
    Serial.printf("[OK] SoftAP started: %s\n", SSID_AP);

    // Setup DNS
    dnsServer.start(DNS_PORT, "*", apIP);
    Serial.println("[OK] DNS server started");

    // Setup Web Server routes
    webServer.on("/", HTTP_GET, [](void) {
        webServer.send(200, "text/html", index_html);
    });

    webServer.on("/submit", HTTP_POST, [](void) {
        if (webServer.hasArg("username")) {
            String username = webServer.arg("username");
            // Sanitize input
            username.trim();
            if (username.length() > 0 && username.length() <= 12) {
                // Send success page
                webServer.send(200, "text/html",
                    "<html><body style='font-family:Arial;text-align:center;padding:40px;background:#2d2d2d;color:white;'>"
                    "<h1 style='color:#00aa00;'>✓ Success!</h1>"
                    "<p>Identity created: <b>" + username + "</b></p>"
                    "<p>Check your wearable device...</p>"
                    "<a href='/' style='color:#007bff;text-decoration:none;'>← Back</a>"
                    "</body></html>");
                sendIdentityToStickC(username);
            } else {
                webServer.send(400, "text/html",
                    "<html><body style='font-family:Arial;text-align:center;padding:40px;background:#2d2d2d;color:white;'>"
                    "<h1 style='color:#ff5555;'>✗ Invalid Input</h1>"
                    "<p>Name must be 2-12 characters</p>"
                    "<a href='/' style='color:#007bff;text-decoration:none;'>← Back</a>"
                    "</body></html>");
            }
        } else {
            webServer.send(400, "text/plain", "Missing username field");
        }
    });

    webServer.onNotFound([](void) {
        webServer.send(200, "text/html", index_html);
    });

    webServer.begin();
    Serial.println("[OK] Web server started on port 80");

    // Initialize ESP-NOW
    initESPNow();

    Serial.println("=== STATION 1 READY ===\n");
}

// ============================================
// MAIN LOOP
// ============================================
void loop() {
    // Handle DNS and HTTP requests
    dnsServer.processNextRequest();
    webServer.handleClient();

    // Handle button presses
    M5.update();

    // Periodic UI refresh
    if (millis() - lastUIUpdate > 10000) {
        drawUI();
        lastUIUpdate = millis();
    }

    delay(10);
}
