#include <M5EPD.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp_now.h>
#include "config.h"

uint8_t atomMAC[] = {0x4C, 0x75, 0x25, 0xAC, 0xBE, 0x18};

typedef struct struct_message {
  char msg[32];
} struct_message;

struct_message outgoing;

M5EPD_Canvas canvas(&M5.EPD);
M5EPD_Canvas status_canvas(&M5.EPD); 

AsyncWebServer paperServer(80);

int selectedChoice = 0;

bool submitted = false;

// Receive trigger action from atom matrix
void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  // --- TRIGGER EVENT ON M5PAPER ---
  Serial.print("receive from matrix");
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

  canvas.setTextSize(4);
  canvas.drawString("CAMT WEB3 CAFE", 100, 50);

  canvas.setTextSize(3);
  canvas.drawRect(0, 130, 540, 80, 15);
  canvas.drawString("1. Coffee......2 CCoin", 30, 155);
  defaultSelectButton(1);

  canvas.drawRect(0, 230, 540, 80, 15);
  canvas.drawString("2. Croissant...3 CCoin", 30, 255);
  defaultSelectButton(2);

  canvas.drawRect(0, 330, 540, 80, 15);
  canvas.drawString("3. Lunch Set...5 CCoin", 30, 355);
  defaultSelectButton(3);

  canvas.drawRect(0, 430, 540, 80, 15);
  canvas.drawString("4. Tea.........2 CCoin", 30, 455);
  defaultSelectButton(4);

  canvas.pushCanvas(0,0, UPDATE_MODE_GC16);
}

void defaultSelectButton(int choice) {
  int ySelector = choice * 100 + 70;
  canvas.fillCircle(490, ySelector, 30, 0);
  canvas.drawCircle(490, ySelector, 30, 15);
}

void selectButton(int choice) {
  int ySelector = choice * 100 + 70;
  canvas.fillCircle(490, ySelector, 30, 15);
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

void handleSystemReset(AsyncWebServerRequest *request) {
  drawMenu();
  updateStatus("");
  submitted = false;
  selectedChoice = 0;
  request->send(200, "text/plain", "M5-Paper S4 reset complete.");
}

void handleGetRequest(AsyncWebServerRequest *request) {
  String receivedUsername = "";
  if (request->hasParam("value", true)) { // POST
    
  } else if (request->hasParam("value", false)) { // GET

  } else {
    request->send(400, "text/plain", "Missing Parameter.");
  }
}

void setup() {
  M5.begin();
    
  Serial.begin(115200);
    
  M5.EPD.SetRotation(90); 
  M5.EPD.Clear(true);
  M5.TP.SetRotation(0);

  drawMenu();

  WiFi.config(IP_PAPER_S4, IP_STATION1_AP, IPAddress(255, 255, 255, 0));
  WiFi.begin(AP_SSID, AP_PASSWORD);

  // ESP-NOW init
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed!");
    return;
  }

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, atomMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  paperServer.on(ENDPOINT_GET_ORDER, HTTP_GET, handleGetRequest);
  paperServer.begin();
}

void loop() {
  if (M5.TP.available()) {
    if (!M5.TP.isFingerUp()) {
      M5.TP.update();
        
      // อ่านค่า X (0-540) และ Y (0-960)
      int x = M5.TP.readFingerX(0);
      int y = M5.TP.readFingerY(0);

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