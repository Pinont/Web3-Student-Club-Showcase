#include <M5EPD.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include "config.h"

// สร้าง Canvas 2 ใบ (ใบใหญ่=เมนู, ใบเล็ก=Status)
M5EPD_Canvas canvas(&M5.EPD);        
M5EPD_Canvas status_canvas(&M5.EPD); 
M5EPD_Canvas choice_canvas(&M5.EPD); 

int selectedChoice = 0;
bool submitted = false;
AsyncWebServer server(80);

String choiceName(int choice) {
    if (choice == 1) {
        return "Walk 1000 steps";
    } else if (choice == 2) {
        return "Recycle bottle";
    } else if (choice == 3) {
        return "Bike 1 km";
    } else if (choice == 4) {
        return "Reuse cup";
    } else {
        return "no choice presets";
    }
}

String choiceCoin(int choice) {
    if (choice == 1) { // walk 1000 step
        return "10";
    } else if (choice == 2 || choice == 4) { // recycle bottle or reuse cup
        return "5";
    } else if (choice == 3) { // bike 1 km
        return "20";
    } else { // no c
        return "0";
    }
}

void drawMenu() {
    canvas.createCanvas(540, 960); 
    
    // Header
    canvas.setTextSize(4);
    canvas.drawString("What did you do today?", 8, 50);

    canvas.setTextSize(3);

    int yRect = 130;
    int yString = 155;
    for (int i = 1; i <= 4; i++) {
        canvas.drawRect(0, yRect, 540, 80, 15);
        String name = String(i) + ". " + choiceName(i);
        canvas.drawString(name, 30, yString);
        String coin = "   --> " + choiceCoin(i) + " CCoin";
        canvas.drawString(coin, 30, yString + 30);
        defaultSelectButton(i);
        yRect += 100;
        yString += 100;
    }

    canvas.fillRect(120, 550, 300, 100, 15);
    canvas.setTextColor(0, 15); 
    canvas.setTextSize(4);
    canvas.drawString("Submit", 200, 585);
    
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
    submitted = false;
    selectedChoice = 0;
    request->send(200, "text/plain", "M5-Paper S3 reset complete.");
}

bool sendReceiveCoin(IPAddress ip, const char* endpoint, const String& body) {
    HTTPClient http;
    String url = String("http://") + ip.toString() + endpoint;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(body);
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
    WiFi.config(IP_PAPER_S3, IP_STATION1_AP, IPAddress(255, 255, 255, 0));
    WiFi.begin(AP_SSID, AP_PASSWORD);

    Serial.println("Connecting to AP...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.print("\nConnected to AP. IP: ");
    Serial.println(WiFi.localIP());

    // ตั้งค่า Server Endpoints
    server.on(ENDPOINT_RESET_GLOBAL, HTTP_POST, handleSystemReset);

    server.begin();

    drawMenu();
}

void loop() {
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
                // ปุ่ม Submit (เช็ค X ให้อยู่ในกรอบ 120-420)
                else if (x >= 550 && x <= 650 && y >= 120 && y <= 420) { // 120 <= y <= 420 && 550 <= x <= 650
                    if (selectedChoice == 0) {
                        updateStatus("Please select first!");
                    } else {
                        updateStatus("Submitting...");
                        String type = "--> " + choiceName(selectedChoice);
                        String coin = "You'll receive: " + choiceCoin(selectedChoice) + " CCoin";
                        String body = "{\"amount\": " + choiceCoin(selectedChoice) + "}";
                        submitted = sendReceiveCoin(IP_STICKC, ENDPOINT_EARN_COIN, body);

                        if (submitted) {
                            sumbitStatus(type, coin);
                        } else {
                            updateStatus("Error sending coin.");
                        }
                    }
                }
            }
            delay(100);
            x = 0;
            y = 0;
        }
    }
}