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
const char* password = "12345678";
const char* stickc_ip = "192.168.4.2";
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

    // ข้อ 1
    // canvas.drawRect(0, 130, 540, 80, 15);
    // canvas.drawString("1. " + choiceName(1), 30, 155); 
    // canvas.drawString("   --> " + choiceCoins(1) + " CCoin", 30, 185);

    // ข้อ 2
    // canvas.drawRect(0, 230, 540, 80, 15);
    // canvas.drawString("2. " + choiceName(2), 30, 255);
    // canvas.drawString("   --> " + choiceCoins(2) + " CCoin", 30, 285);

    // ข้อ 3
    // canvas.drawRect(0, 330, 540, 80, 15);
    // canvas.drawString("3. " + choiceName(3), 30, 355);
    // canvas.drawString("   --> " + choiceCoins(3) + " CCoin", 30, 385);

    // ข้อ 4
    // canvas.drawRect(0, 430, 540, 80, 15);
    // canvas.drawString("4. " + choiceName(4), 30, 455);
    // canvas.drawString("   --> " + choiceCoins(4) + " CCoin", 30, 485);

    int yRect = 130;
    int yString = 155;
    for (int i = 1; i <= 4; i++) {
        canvas.drawRect(0, yRect, 540, 80, 15);
        String name = String(i) + ". " + choiceName(i);
        canvas.drawString(name, 30, yString);
        String coin = "   --> " + choiceCoin(i) + " CCoin";
        canvas.drawString(coin, 30, yString + 30);
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
    // int yRect = choice * 100 + 30;
    // int yString = choice * 100 + 55;
    // choice_canvas.createCanvas(540, 80);
    // choice_canvas.fillRect(0, yRect, 540, 80, 15);
    // choice_canvas.setTextColor(0, 15);
    // choice_canvas.setTextSize(3);
    // choice_canvas.drawString(String(choice) + ". " + choiceName(choice), 30, yString);
    // choice_canvas.drawString("   --> " + choiceCoin(choice) + " CCoin", 30, yString + 30);
    // choice_canvas.pushCanvas(0, yRect, UPDATE_MODE_DU4);
    // Serial.println("selectCanvas: " + choiceName(choice));
}

void defaultSelectButton(int choice) {
    // int yRect = choice * 100 + 30;
    // int yString = choice * 100 + 55;
    // String name = String(choice) + ". " + choiceName(choice);
    // String coin = "   --> " + choiceCoin(choice) + " CCoin";
    // choice_canvas.createCanvas(540, 80);
    // choice_canvas.drawRect(0, yRect, 540, 80, 15);
    // choice_canvas.drawString(name, 30, yString);
    // choice_canvas.drawString(coin, 30, yString + 30);
    // choice_canvas.pushCanvas(0, yRect, UPDATE_MODE_DU4);
}

bool sendReceiveCoin(String coin) {
    HTTPClient http;

    String url = "http://" + String(stickc_ip) + ":" + String(port) + "/add_coin";
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String postData = "value=" + coin;

    int code = http.POST(postData);
    String response = http.getString();
    http.end();
    if (code == 200) {
        Serial.println("Sent OK");
        return true;
    }
    Serial.println(String(code) + " Error: " +  response);
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
                if (x >= 130 && x <= 229) {
                    if (selectedChoice > 0) {
                        defaultSelectButton(selectedChoice);
                    }
                    selectedChoice = 1;
                    selectButton(selectedChoice);
                    updateStatus("Selected: " + choiceName(selectedChoice));
                }
                else if (x >= 230 && x <= 329) {
                    if (selectedChoice > 0) {
                        defaultSelectButton(selectedChoice);
                    }
                    selectedChoice = 2;
                    selectButton(selectedChoice);
                    updateStatus("Selected: " + choiceName(selectedChoice));
                }
                else if (x >= 330 && x <= 410) {
                    if (selectedChoice > 0) {
                        defaultSelectButton(selectedChoice);
                    }
                    selectedChoice = 3;
                    selectButton(selectedChoice);
                    updateStatus("Selected: " + choiceName(selectedChoice));
                }
                else if (x >= 430 && x <= 510) {
                    if (selectedChoice > 0) {
                        defaultSelectButton(selectedChoice);
                    }
                    selectedChoice = 4;
                    selectButton(selectedChoice);
                    updateStatus("Selected: " + choiceName(selectedChoice));
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