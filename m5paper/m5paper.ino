#include <M5EPD.h>
#include <WiFi.h>
#include <HTTPClient.h>

// สร้าง Canvas 2 ใบ (ใบใหญ่=เมนู, ใบเล็ก=Status)
M5EPD_Canvas canvas(&M5.EPD);

void drawMenu() {
  canvas.createCanvas(540, 960);

  // CAMT WEB3 CAFE
  canvas.setTextSize(4);
  canvas.drawString("CAMT WEB3 CAFE", 100, 50);

  canvas.setTextSize(3);

  // 1. Coffee........................2 CCoin
  canvas.drawRect(0, 130, 540, 80, 15);
  canvas.drawString("1. Coffee..............2 CCoin", 30, 155);

  // 2. Croissant.....................3 CCoin
  canvas.drawRect(0, 230, 540, 80, 15);
  canvas.drawString("2. Croissant...........3 CCoin", 30, 255);

  // 3. Lunch Set.....................5 CCoin
  canvas.drawRect(0, 330, 540, 80, 15);
  canvas.drawString("3. Lunch Set...........5 CCoin", 30, 355);

  // 4. Tea...........................2 CCoin
  canvas.drawRect(0, 430, 540, 80, 15);
  canvas.drawString("4. Tea.................2 CCoin", 30, 455);

  canvas.pushCanvas(0,0, UPDATE_MODE_GC16);
}

void setup() {
  M5.begin();
    
  Serial.begin(115200);
    
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
      delay(100);
      x = 0;
      y = 0;
    }
  }
}