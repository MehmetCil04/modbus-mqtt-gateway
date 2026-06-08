#pragma once

#include <Arduino.h>

class DisplayUI {
   public:
    void begin();
    void showMessage(const String& title, const String& body);
    void showStatus(bool wifiOk, bool mqttOk, uint32_t okCount, uint32_t errCount);

   private:
    bool m_ok = false;
};
