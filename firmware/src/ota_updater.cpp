#include "ota_updater.h"

#include <ArduinoOTA.h>

namespace OtaUpdater {

void begin() {
    ArduinoOTA.setHostname("modbus-gateway");
    ArduinoOTA.onStart([]() { Serial.println("[OTA] start"); });
    ArduinoOTA.onEnd([]() { Serial.println("[OTA] end"); });
    ArduinoOTA.onError([](ota_error_t e) { Serial.printf("[OTA] error %u\n", e); });
    ArduinoOTA.begin();
}

void loop() {
    ArduinoOTA.handle();
}

}  // namespace OtaUpdater
