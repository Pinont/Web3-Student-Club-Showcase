#include <M5Atom.h>
#include <esp_now.h>
#include <WiFi.h>
#include <driver/i2s.h>
#include <esp_wifi.h> // เพิ่มเพื่อบังคับ Channel

#define CONFIG_I2S_BCK_PIN 19
#define CONFIG_I2S_LRCK_PIN 33
#define CONFIG_I2S_DATA_PIN 22
#define CONFIG_I2S_DATA_IN_PIN 23

// ตั้งค่า I2S
void InitI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB, // ใช้ MSB เสียงจะดีกว่า
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = CONFIG_I2S_BCK_PIN,
        .ws_io_num = CONFIG_I2S_LRCK_PIN,
        .data_out_num = CONFIG_I2S_DATA_PIN,
        .data_in_num = CONFIG_I2S_DATA_IN_PIN
    };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_set_clk(I2S_NUM_0, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

// สร้างเสียง Beep
void PlayBeep(int freq, int duration_ms) {
    size_t bytes_written;
    int samples = 44100 * duration_ms / 1000;
    int16_t *buffer = (int16_t *)malloc(samples * 2);
    if (!buffer) return;
    
    int period = 44100 / freq; 
    int volume = 2000; 

    for (int i = 0; i < samples; i++) {
        if ((i % period) < (period / 2)) buffer[i] = volume;
        else buffer[i] = -volume;
    }
    
    i2s_write(I2S_NUM_0, buffer, samples * 2, &bytes_written, portMAX_DELAY);
    free(buffer);

    // ล้างท่อเสียง
    int16_t silence[1024] = {0};
    for (int k = 0; k < 3; k++) {
        i2s_write(I2S_NUM_0, silence, sizeof(silence), &bytes_written, portMAX_DELAY);
    }
}

bool triggerSound = false;

// Callback รับข้อมูล
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    if (len > 0 && *incomingData == 1) {
        triggerSound = true; 
    }
}

void setup() {
    M5.begin(true, false, true);
    InitI2S();
    Serial.begin(115200);

    // 1. โชว์ MAC Address ครั้งเดียวพอ (ดูใน Serial Monitor)
    WiFi.mode(WIFI_STA);
    Serial.println("\n--------------------------------");
    Serial.print("MY MAC ADDRESS:  ");
    Serial.println(WiFi.macAddress());
    Serial.println("--------------------------------");

    // 2. ⚠️ บังคับ Channel 1 (เพื่อให้ตรงกับ Matrix) ⚠️
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    // 3. Test Sound
    Serial.println("System Start: Testing Beep...");
    M5.dis.drawpix(0, 0xFFFFFF); 
    PlayBeep(2000, 100); 
    M5.dis.drawpix(0, 0x0000FF);
    
    if (esp_now_init() != ESP_OK) return;
    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
    M5.update();

    // ปุ่มกดเทสเสียง
    if (M5.Btn.wasPressed()) {
        Serial.println("Button Clicked!");
        triggerSound = true; 
    }
    
    // เมื่อได้รับคำสั่งจาก Matrix
    if (triggerSound) {
        M5.dis.drawpix(0, 0x00FF00); // สีเขียว
        
        Serial.println("Matrix Command -> Beep!");
        PlayBeep(1500, 100); 
        delay(50);
        PlayBeep(2500, 200); 
        
        triggerSound = false; 
        M5.dis.drawpix(0, 0x0000FF); // กลับเป็นน้ำเงิน
    }
}