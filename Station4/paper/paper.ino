#include <M5EPD.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <esp_now.h>
#include "config.h"

// สร้าง Canvas 2 ใบ (ใบใหญ่=เมนู, ใบเล็ก=Status)
M5EPD_Canvas canvas(&M5.EPD);        
M5EPD_Canvas status_canvas(&M5.EPD); 
M5EPD_Canvas choice_canvas(&M5.EPD); 

uint8_t atomMAC[] = {0x4C, 0x75, 0x25, 0xAC, 0xBE, 0x18};

typedef struct struct_message {
  char msg[32];
} struct_message;

struct_message outgoing;

int selectedChoice = 0;
bool submitted = false;
AsyncWebServer server(80);

struct Activity {
    String name;
};

Activity activities[] = {
    {"Coffee       2 CCoin"},
    {"Croissant    3 CCoin"},
    {"Lunch Set    5 CCoin"},
    {"Tea          2 CCoin"}
};

void sendCommand(const char *cmd);
void updateStatus(String msg);

// Receive trigger action from atom matrix
void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  // --- TRIGGER EVENT ON M5PAPER ---
  Serial.println("receive from matrix");
  if (selectedChoice != 0) {
      String msg = String(selectedChoice);
      sendCommand(msg.c_str());
      updateStatus("Sending Order: " + msg);
  } else {
      updateStatus("Please Select Order");
  }
}

// Send number of order to atom matrix
void sendCommand(const char *cmd) {
  strcpy(outgoing.msg, cmd);
  esp_now_send(atomMAC, (uint8_t *)&outgoing, sizeof(outgoing));
  Serial.print("Sent: ");
  Serial.println(cmd);
}

void drawMenu() {

    canvas.createCanvas(540, 960); 
    
    // Header
    canvas.setTextSize(4);
    canvas.drawString("CAMT WEB3 CAFE", 100, 50);

    canvas.setTextSize(3);

    int yRect = 130;
    int yString = 155;
    for (int i = 1; i <= 4; i++) {
        canvas.drawRect(0, yRect, 540, 80, 15);
        String name = String(i) + ". " + activities[i-1].name;
        canvas.drawString(name, 30, yString);
        defaultSelectButton(i);
        yRect += 100;
        yString += 100;
    }
    
    canvas.setTextColor(15, 0); 

    canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
}

void updateStatus(String msg) {
    status_canvas.createCanvas(540, 100); 
    status_canvas.fillCanvas(0);          
    status_canvas.setTextSize(3);
    status_canvas.drawString(msg, 20, 20);

    status_canvas.pushCanvas(0, 700, UPDATE_MODE_DU4); 
}

void sumbitStatus(String msg1, String msg2) {
    status_canvas.createCanvas(540, 100); 
    status_canvas.fillCanvas(0);          
    status_canvas.setTextSize(3);
    status_canvas.drawString(msg1, 20, 20);
    status_canvas.drawString(msg2, 23, 50);

    status_canvas.pushCanvas(0, 700, UPDATE_MODE_DU4);

    Serial.println("Submitted");
}

void selectButton(int choice) {
    int ySelector = choice * 100 + 70;
    canvas.fillCircle(490, ySelector, 30, 15);
}

void defaultSelectButton(int choice) {
    int ySelector = choice * 100 + 70;
    canvas.fillCircle(490, ySelector, 30, 0);
    canvas.drawCircle(490, ySelector, 30, 15);
}

void handleSystemReset(AsyncWebServerRequest *request) {
    drawMenu();
    updateStatus("");
    submitted = false;
    selectedChoice = 0;
    Serial.println("Received reset signal from Core");
    request->send(200, "text/plain", "M5-Paper S3 reset complete.");
}

bool sendReceiveCoin(String coin_value) {
    HTTPClient http;
    String url = "http://" + IP_STICKC.toString() + ENDPOINT_EARN_COIN;
    Serial.println("Sending " + coin_value + " to " + url);
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String postData = "amount=" + coin_value; 

    int code = http.POST(postData);
    String response = http.getString();
    http.end();
    if (code == 200) {
        Serial.println("Sent OK");
        return true;
    }
    Serial.println(String(code) + " Error: " + response);
    return false;
}

void setup() {
    M5.begin();
    M5.EPD.SetRotation(90); // Landscape
    Serial.begin(115200);

    // กำหนด Static IP
    WiFi.config(IP_PAPER_S4, IP_STATION1_AP, IPAddress(255, 255, 255, 0));
    WiFi.begin(AP_SSID, AP_PASSWORD);

    Serial.println("Connecting to AP...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.print("\nConnected to AP. IP: ");
    Serial.println(WiFi.localIP());

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_recv_cb(onDataRecv);

    // Register peer
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, atomMAC, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("Failed to add peer");
        return;
    }

    // ตั้งค่า Server Endpoints
    server.on(ENDPOINT_RESET_GLOBAL, HTTP_POST, handleSystemReset);
    server.on(ENDPOINT_HEARTBEAT, HTTP_GET, [] (AsyncWebServerRequest *r) {
        r->send(200, "text/plain", "OK");
    });

    server.begin();

    drawMenu();
}

void loop() {
    M5.update();

    if (M5.TP.available()) {
        if (!M5.TP.isFingerUp()) {
            M5.TP.update();
            
            // อ่านค่า X (0-540) และ Y (0-960)
            int x = M5.TP.readFingerX(0);
            int y = M5.TP.readFingerY(0);

            // Serial.printf("X: %d, Y: %d\n", x, y);
            if (!submitted) {
                if (selectedChoice != 1 && x >= 130 && x <= 229) {
                    if (selectedChoice > 0) {
                        defaultSelectButton(selectedChoice);
                    }
                    selectedChoice = 1;
                    selectButton(selectedChoice);
                    canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
                }
                else if (selectedChoice != 2 && x >= 230 && x <= 329) {
                    if (selectedChoice > 0) {
                        defaultSelectButton(selectedChoice);
                    }
                    selectedChoice = 2;
                    selectButton(selectedChoice);
                    canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
                }
                else if (selectedChoice != 3 && x >= 330 && x <= 410) {
                    if (selectedChoice > 0) {
                        defaultSelectButton(selectedChoice);
                    }
                    selectedChoice = 3;
                    selectButton(selectedChoice);
                    canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
                }
                else if (selectedChoice != 4 && x >= 430 && x <= 510) {
                    if (selectedChoice > 0) {
                        defaultSelectButton(selectedChoice);
                    }
                    selectedChoice = 4;
                    selectButton(selectedChoice);
                    canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
                }
            }
            delay(100);
        }
    }
}