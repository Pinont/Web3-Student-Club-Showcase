#include <M5Atom.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// --- ตั้งค่าระยะที่ต้องการ ---
const int RSSI_THRESHOLD = -45; 

// หน่วงเวลาห้ามส่งซ้ำ (2000ms = 2 วินาที)
const int COOLDOWN_TIME = 5000;  

BLEUUID targetUUID = BLEUUID("1234");

// MAC Address ของ Echo 
uint8_t echoAddress[] = {0x90, 0x15, 0x06, 0xFD, 0xF2, 0xF8};

// ✅ 
int checkIcon[] = {
    15, // หางสั้น (ซ้ายล่าง)
    21, // จุดกลับตัว (ล่างสุด)
    17, // เส้นเฉียงขึ้น
    13, // เส้นเฉียงขึ้น
    9   // ปลายหางยาว (ขวาบน)
};

BLEScan* pBLEScan;
unsigned long lastTriggerTime = 0; 

void setup() {
    M5.begin(true, false, true); 
    delay(10);

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    for (int i = 0; i < 6; i++) {
        peerInfo.peer_addr[i] = echoAddress[i]; 
    }
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("Failed to add peer");
        return;
    }

    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); 
    pBLEScan->setActiveScan(true);   
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);

    Serial.println("System Ready: Scanning...");
    M5.dis.fillpix(0x0000FF); // เริ่มต้น: สีน้ำเงิน
}

void loop() {
    M5.update();
    
    //แก้: ใช้ Pointer (*) เพื่อแก้ Error เก่า
    BLEScanResults *foundDevices = pBLEScan->start(1, false);
    
    bool foundTarget = false;
    int targetRSSI = -999;

    for (int i = 0; i < foundDevices->getCount(); i++) {
        BLEAdvertisedDevice device = foundDevices->getDevice(i);
        
        // เช็ค UUID
        if (device.haveServiceUUID() && device.isAdvertisingService(targetUUID)) {
            targetRSSI = device.getRSSI();
            Serial.printf("Target Found! RSSI: %d\n", targetRSSI);
            
            if (targetRSSI > RSSI_THRESHOLD) {
                foundTarget = true;
            }
        }
    }
    
    // --- ตัดสินใจ ---
    if (foundTarget) {
        
        // ---ติ๊กถูก (Checkmark) ---
        M5.dis.clear(); // ล้างสีเดิมก่อน
        for (int i = 0; i < 5; i++) {
            M5.dis.drawpix(checkIcon[i], 0x00FF00); // วาดจุดสีเขียวตามแบบแปลน
        }
        // -------------------------------------------

        // เช็ค Cooldown ก่อนส่งคำสั่ง
        if (millis() - lastTriggerTime > COOLDOWN_TIME) {
            
            Serial.println(">>> UNLOCK! Sending to Echo <<<");

            uint8_t data = 1; 
            esp_now_send(echoAddress, &data, sizeof(data));

            lastTriggerTime = millis(); 
        } 

    } else {
        // ไม่เจอ -> สีน้ำเงิน 🔵
        M5.dis.fillpix(0x0000ff); 
        Serial.println("Searching...");
    }

    pBLEScan->clearResults(); 
}
