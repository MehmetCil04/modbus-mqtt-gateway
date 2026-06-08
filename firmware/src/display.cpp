#include "display.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Wire.h>

namespace {
constexpr uint8_t kAddr = 0x3C;
constexpr int kW = 128;
constexpr int kH = 64;
Adafruit_SSD1306 s_oled(kW, kH, &Wire, -1);
}  // namespace

void DisplayUI::begin() {
    Wire.begin();
    m_ok = s_oled.begin(SSD1306_SWITCHCAPVCC, kAddr);
    if (!m_ok) {
        Serial.println("[OLED] not found");
        return;
    }
    s_oled.clearDisplay();
    s_oled.setTextSize(1);
    s_oled.setTextColor(SSD1306_WHITE);
    s_oled.display();
}

void DisplayUI::showMessage(const String& title, const String& body) {
    if (!m_ok) return;
    s_oled.clearDisplay();
    s_oled.setCursor(0, 0);
    s_oled.println(title);
    s_oled.setCursor(0, 16);
    s_oled.println(body);
    s_oled.display();
}

void DisplayUI::showStatus(bool wifiOk, bool mqttOk, uint32_t okCount, uint32_t errCount) {
    if (!m_ok) return;
    s_oled.clearDisplay();
    s_oled.setCursor(0, 0);
    s_oled.printf("WiFi:%s MQTT:%s\n", wifiOk ? "OK" : "--", mqttOk ? "OK" : "--");
    s_oled.printf("IP : %s\n", WiFi.localIP().toString().c_str());
    s_oled.printf("OK : %u\n", (unsigned)okCount);
    s_oled.printf("ERR: %u\n", (unsigned)errCount);
    s_oled.display();
}
