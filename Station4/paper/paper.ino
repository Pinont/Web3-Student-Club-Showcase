#include <M5EPD.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "config.h"

// สร้าง Canvas 2 ใบ (ใบใหญ่=เมนู, ใบเล็ก=Status)
M5EPD_Canvas canvas(&M5.EPD);        
M5EPD_Canvas status_canvas(&M5.EPD); 

uint8_t atomMAC[] = {0x4C, 0x75, 0x25, 0xAC, 0xBE, 0x18};
uint8_t stickc1MAC[]  = {0x00, 0x4B, 0x12, 0xC4, 0x2D, 0xF8};
uint8_t stickc2MAC[]  = {0x00, 0x4b, 0x12, 0xC4, 0x35, 0x48};
uint8_t echoMAC[]   = {0x90, 0x15, 0x06, 0xFA, 0xE7, 0x70};

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
    {"Coffee.......2 CCoin"},
    {"Croissant....3 CCoin"},
    {"Lunch Set....5 CCoin"},
    {"Tea..........2 CCoin"}
};

// Send number of order to atom matrix
void sendCommand(uint8_t *macAddr, const char *cmd) {
  strcpy(outgoing.msg, cmd);
  esp_now_send(macAddr, (uint8_t *)&outgoing, sizeof(outgoing));
  Serial.print("Sent: ");
  Serial.println(cmd);
}

void drawMenu() {

    canvas.createCanvas(540, 960); 
    
    canvas.fillCanvas(0);
    
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
    String msg = String(-1);
    sendCommand(atomMAC, msg.c_str());
    sendCommand(echoMAC, msg.c_str());
    Serial.println("Received reset signal from Core");
    request->send(200, "text/plain", "M5-Paper S4 reset complete.");
    ESP.restart();
}

void addPeer(uint8_t *macAddr) {
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, macAddr, 6);
    peerInfo.channel = WiFi.channel();
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
}

void setup() {
    M5.begin();
    M5.EPD.SetRotation(90); // Landscape
    Serial.begin(115200);

    // กำหนด Static IP
    WiFi.config(IP_PAPER_S4, IP_STATION1_AP, IPAddress(255, 255, 255, 0));
    WiFi.setTxPower(WIFI_POWER_19_5dBm); // Recommended to set power before connect
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE); // Force channel 1
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

    addPeer(atomMAC);
    addPeer(stickc1MAC);
    addPeer(stickc2MAC);
    addPeer(echoMAC);

    // ตั้งค่า Server Endpoints
    server.on(ENDPOINT_RESET_GLOBAL, HTTP_POST, handleSystemReset);
    server.on(ENDPOINT_GET_ORDER, HTTP_GET, [] (AsyncWebServerRequest *r) {
        if (selectedChoice == 0) {
            r->send(201, "text/plain", "Choice not selected");
            return;
        }
        String msg = "";
        if (selectedChoice == 0) {
            msg = "48";
        } else if (selectedChoice == 1) {
            msg = "49";
        } else if (selectedChoice == 2) {
            msg = "50";
        } else if (selectedChoice == 3) {
            msg = "51";
        } else if (selectedChoice == 4) {
            msg = "52";
        }
        sendCommand(echoMAC, msg.c_str());
        sendCommand(stickc1MAC, msg.c_str());
        sendCommand(stickc2MAC, msg.c_str());

        r->send(200, "text/plain", "OK");
    });
    server.on(ENDPOINT_HEARTBEAT, HTTP_GET, [] (AsyncWebServerRequest *r) {
        r->send(200, "text/plain", "OK");
    });

    server.begin();

    drawMenu();
}

void touchAction() {
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
        }
    }
}

void loop() {
    M5.update();
    touchAction();
    delay(100);
}