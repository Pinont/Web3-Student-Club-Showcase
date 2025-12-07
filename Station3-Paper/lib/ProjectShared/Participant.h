// Participant.h: Data Structure สำหรับจัดการข้อมูลผู้เข้าร่วม (Username, Status, CCoin)

#ifndef PARTICIPANT_H
#define PARTICIPANT_H

#include <Arduino.h>

class Participant {
public:
    String Username;
    bool isAuthenticated;
    int CCoin_Balance;
    String alertText;

    Participant() {
        // ค่าเริ่มต้นเมื่อเริ่มต้นระบบหรือทำการ Reset
        reset();
    }

    void reset() {
        Username = "";
        isAuthenticated = false;
        CCoin_Balance = 0;
        alertText = "-";
    }

    String getAuthStatus() {
        return isAuthenticated ? "✓" : "X";
    }

    // ฟังก์ชันสำหรับส่งข้อมูลในรูปแบบ JSON
    String toJSON() {
        String json = "{";
        json += "\"username\": \"" + Username + "\",";
        json += "\"isAuthenticated\": " + String(isAuthenticated ? "true" : "false") + ",";
        json += "\"ccoin_balance\": " + String(CCoin_Balance) + ",";
        json += "\"alert_text\": \"" + alertText + "\"";
        json += "}";
        return json;
    }

    // ฟังก์ชันสำหรับอัปเดตข้อมูลจาก JSON (ใช้เมื่อรับ Request)
    void updateFromJSON(const String& json) {
        // ในโปรเจกต์นี้จะทำการแยก Parser JSON ในแต่ละอุปกรณ์เพื่อให้ง่ายต่อการจัดการ
        // แต่โครงสร้างนี้ช่วยให้รู้ว่ามีข้อมูลอะไรบ้าง
    }
};

#endif // PARTICIPANT_H