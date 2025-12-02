#include <M5EPD.h>

// สร้าง Canvas 2 ใบ (ใบใหญ่=เมนู, ใบเล็ก=Status)
M5EPD_Canvas canvas(&M5.EPD);        
M5EPD_Canvas status_canvas(&M5.EPD); 

int selectedChoice = 0;

void drawMenu() {
    canvas.createCanvas(540, 960); 
    
    // Header
    canvas.setTextSize(4);
    canvas.drawString("What did you do?", 30, 50);

    canvas.setTextSize(3);
    
    
    // ข้อ 1
    canvas.drawRect(0, 130, 540, 80, 15);
    canvas.drawString("1. Walk 1000 steps", 30, 155); 
    canvas.drawString("   --> 10 CCoin", 30, 185);

    // ข้อ 2
    canvas.drawRect(0, 230, 540, 80, 15);
    canvas.drawString("2. Recycle bottle", 30, 255);
    canvas.drawString("   --> 5 CCoin", 30, 285);

    // ข้อ 3
    canvas.drawRect(0, 330, 540, 80, 15);
    canvas.drawString("3. Bike 1 km", 30, 355);
    canvas.drawString("   --> 20 CCoin", 30, 385);

    // ข้อ 4
    canvas.drawRect(0, 430, 540, 80, 15);
    canvas.drawString("4. Reuse cup", 30, 455);
    canvas.drawString("   --> 5 CCoin", 30, 485);

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
    status_canvas.drawString("Status: " + msg, 20, 20);

    status_canvas.pushCanvas(0, 700, UPDATE_MODE_DU4); 

    Serial.print("Status: ");
    Serial.println(msg);
}

void setup() {
    M5.begin();
    
    Serial.begin(115200);
    
    M5.EPD.SetRotation(90); 
    M5.EPD.Clear(true);
    M5.TP.SetRotation(0); // หมุนระบบสัมผัสให้ตรงกัน

    drawMenu();     
}

void loop() {
    if (M5.TP.available()) {
        if (!M5.TP.isFingerUp()) {
            M5.TP.update();
            
            // อ่านค่า X (0-540) และ Y (0-960)
            int x = M5.TP.readFingerX(0);
            int y = M5.TP.readFingerY(0);

            Serial.printf("X: %d, Y: %d\n", x, y);

            if (x >= 130 && x <= 229) {
                selectedChoice = 1;
                updateStatus("Selected: Walk");
            }
            else if (x >= 230 && x <= 329) {
                selectedChoice = 2;
                updateStatus("Selected: Recycle");
            }
            else if (x >= 330 && x <= 410) {
                selectedChoice = 3;
                updateStatus("Selected: Bike");
            }
            else if (x >= 430 && x <= 510) {
                selectedChoice = 4;
                updateStatus("Selected: Reuse Cup");
            }
            // ปุ่ม Submit (เช็ค X ให้อยู่ในกรอบ 120-420)
            else if (x >= 550 && x <= 650 && y >= 120 && y <= 420) {
                if (selectedChoice == 0) {
                    updateStatus("Please select first!");
                } else {
                    updateStatus("Submitting...");
                    delay(1000);
                    updateStatus("Sent Successfully!");
                    selectedChoice = 0;
                }
            }
            delay(100);
        }
    }
}