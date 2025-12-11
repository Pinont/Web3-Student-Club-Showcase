#include <M5Unified.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <string.h>


// --- 1. CONFIGURATION: MAC ADDRESSES ---
uint8_t matrixMAC[] = {0x4C, 0x75, 0x25, 0xAC, 0xBE, 0x18};

// --- 3. SOUND SEQUENCER VARIABLES ---
const int shortBeepDuration = 200;
const int longBeepDuration  = 700;
const int beepFreq          = 1600; 
const int pauseDuration     = 100;  

enum SoundState { 
    IDLE, 
    BEEP1_START, BEEP1_WAIT, BEEP1_PAUSE,
    BEEP3_START, BEEP3_WAIT, 
    DONE,        
    RESET_WAIT   
};

SoundState currentSoundState = IDLE;
unsigned long stateChangeTime = 0;

//ฟังก์ชันทำงานเมื่อได้รับข้อมูลกลับมาจากPaper
void OnDataRecv(const esp_now_recv_info_t * info, const uint8_t *incomingData, int len) {
    Serial.print("receive from matrix\nReceived: ");
    Serial.print(incomingData[0]);
    Serial.println();

    if (incomingData[0] == 45) {
        Serial.println("Receive reset trigger from Core.");
        ESP.restart();
    }
    Serial.printf("Recieve Trigger event from matrix");

    // Trigger the sound sequence upon receiving data
    currentSoundState = BEEP1_START;
}

void addPeer(uint8_t *macAddr) {
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, macAddr, 6);
    peerInfo.channel = 1;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
}

// ------------------------------------------------------
// SETUP
// ------------------------------------------------------
void setup() {
    auto cfg = M5.config();
    
    // --- 1. ตั้งค่าพื้นฐาน ---
    cfg.serial_baudrate = 115200; 
    
    // สำคัญมาก: Atom Echo ต้องเปิด output_power เพื่อจ่ายไฟให้ลำโพง (GPIO21/25)
    cfg.output_power = true;      
    
    // *** ลบบรรทัด cfg.external_speaker.atomic_echo = true; ออกครับ ***
    // ให้ M5Unified ตรวจสอบ Board อัตโนมัติ (มันฉลาดพอจะรู้ว่าเป็น Atom Echo)

    M5.begin(cfg);
    
    // --- 2. ตั้งค่าเสียง ---
    M5.Speaker.begin(); // สั่งเริ่มระบบเสียงให้ชัวร์
    M5.Speaker.setVolume(200);
    
    // --- 3. TEST SOUND (ทดสอบทันทีที่เปิดเครื่อง) ---
    // ถ้าบรรทัดนี้ไม่ดัง แสดงว่าฮาร์ดแวร์มีปัญหา หรือไฟไม่พอ
    Serial.println("Testing Speaker...");
    M5.Speaker.tone(1000, 500); 
    delay(1000); // รอฟังเสียง
    M5.Speaker.tone(2000, 500);
    
    WiFi.mode(WIFI_STA);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    
    Serial.println("\n--- M5Atom Echo Started ---");
    Serial.print("MAC: "); Serial.println(WiFi.macAddress());

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        ESP.restart();
    }

    esp_now_register_recv_cb(OnDataRecv);

    auto addPeer = [](const uint8_t* addr) {
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, addr, 6);
        peerInfo.channel = 1; 
        peerInfo.encrypt = false;
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.println("Failed to add peer");
        }
    };

    addPeer(matrixMAC);
}

// ------------------------------------------------------
// MAIN LOOP
// ------------------------------------------------------
void loop() {
    M5.update();
    unsigned long currentTime = millis();

    if (currentSoundState != IDLE) {
        switch (currentSoundState) {
            
            // --- เสียงที่ 1 (สั้น) ---
            case BEEP1_START:
                M5.Speaker.tone(beepFreq); // สั่งดังค้างไว้เลย ไม่ต้องใส่ duration
                stateChangeTime = currentTime;
                currentSoundState = BEEP1_WAIT;
                break;

            case BEEP1_WAIT:
                if (currentTime - stateChangeTime >= shortBeepDuration) {
                    M5.Speaker.stop(); // *** สั่งหยุดเสียงเองเมื่อครบเวลา ***
                    stateChangeTime = currentTime;
                    currentSoundState = BEEP1_PAUSE;
                }
                break;

            case BEEP1_PAUSE:
                if (currentTime - stateChangeTime >= pauseDuration) {
                    currentSoundState = BEEP3_START;
                }
                break;

            // --- เสียงที่ 3 (ยาว) ---
            case BEEP3_START:
                M5.Speaker.tone(beepFreq); // สั่งดัง
                stateChangeTime = currentTime;
                currentSoundState = BEEP3_WAIT;
                break;
                
            case BEEP3_WAIT:
                if (currentTime - stateChangeTime >= longBeepDuration) {
                    M5.Speaker.stop(); // *** สั่งหยุด ***
                    currentSoundState = DONE;
                }
                break;

            case DONE:
                stateChangeTime = currentTime;
                currentSoundState = RESET_WAIT; 
                break;

            case RESET_WAIT:
                if (currentTime - stateChangeTime >= 2000) {
                    M5.Display.fillScreen(0x0000FF); 
                    currentSoundState = IDLE;
                    Serial.println(">> READY <<");
                }
                break;
        }
    }

    // Manual Trigger
    if (M5.BtnA.wasPressed() && currentSoundState == IDLE) {
        Serial.println("Manual Button Trigger!");
        currentSoundState = BEEP1_START;
    }
}