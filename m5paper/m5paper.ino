#include <M5EPD.h>
#include <WiFi.h>
#include <HTTPClient.h>

// สร้าง Canvas 2 ใบ (ใบใหญ่=เมนู, ใบเล็ก=Status)
M5EPD_Canvas canvas(&M5.EPD);        
M5EPD_Canvas status_canvas(&M5.EPD); 
M5EPD_Canvas choice_canvas(&M5.EPD); 

int selectedChoice = 0;
bool submitted = false;

const char* ssid = "Web3Showcase_AP";
const char* password = NULL;
char* stickc_ip = "192.168.4.2";
const char* port = "88";

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

bool sendReceiveCoin(String coin_value) {
    HTTPClient http;
    String url = "http://" + String(stickc_ip) + ":" + String(port) + "/set_coins";
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String postData = "coins=" + coin_value; 

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
    
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    
    M5.EPD.SetRotation(90); 
    M5.EPD.Clear(true);
    M5.TP.SetRotation(0); // หมุนระบบสัมผัสให้ตรงกัน

    drawMenu();     

    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nConnected!");
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
                        submitted = sendReceiveCoin(choiceCoin(selectedChoice));
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