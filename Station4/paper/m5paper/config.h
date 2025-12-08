#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- SoftAP Configuration (Hosted by Station 1) ---
const char* AP_SSID = "Web3_Showcase_AP";
const char* AP_PASSWORD = NULL; // Open network for easy Captive Portal

// --- IP Address Mapping ---
// Gateway (Station 1) must be x.x.x.1
IPAddress IP_STATION1_AP(192, 168, 4, 1);     // Core2: Identity & AP
IPAddress IP_STATION2_MON(192, 168, 4, 2);    // Core2: Auth Monitor
IPAddress IP_RESET_MON(192, 168, 4, 3);       // Core Basic: System Monitor & Reset

IPAddress IP_STICKC(192, 168, 4, 10);         // Wearable
IPAddress IP_ATOM_MATRIX(192, 168, 4, 20);    // S2/S4 Sensor
IPAddress IP_ATOM_ECHO(192, 168, 4, 30);      // Sound
IPAddress IP_PAPER_S3(192, 168, 4, 40);       // Earn
IPAddress IP_PAPER_S4(192, 168, 4, 50);       // Spend

IPAddress NETMASK(255, 255, 255, 0);

// --- Endpoints ---
#define ENDPOINT_HEARTBEAT "/heartbeat"
#define ENDPOINT_SET_USER "/set_user"
#define ENDPOINT_SET_AUTH "/set_auth"
#define ENDPOINT_RESET_GLOBAL "/reset_global"

// [เพิ่มเติม] เพิ่ม Endpoints สำหรับ Earn และ Spend ที่หายไป
#define ENDPOINT_EARN_COIN "/earn_coin"
#define ENDPOINT_SPEND_COIN "/spend_coin"

// [เพิ่มเติม] Endpoints สำหรับ Atom Matrix (เผื่อใช้)
#define ENDPOINT_GET_ORDER "/get_order"

#endif // CONFIG_H