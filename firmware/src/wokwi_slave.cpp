#include "wokwi_slave.h"

#ifdef WOKWI_BUILD

#include <Arduino.h>
#include <ModbusRTU.h>

// Pin map (Wokwi-only):
//   UART1 RX = GPIO5    (slave receives master's TX)
//   UART1 TX = GPIO18   (slave responds, master's RX picks it up)
// In diagram.json these are cross-wired to GPIO17/16 on the same board.
//
// Register map mirrors slave-firmware (and simulators/modbus_slave.py):
//   addr 0     uint16  voltage      0.1  V / LSB
//   addr 1-2   uint32  current      0.001 A / LSB  (big-endian word order)
//   addr 3-4   uint32  power        0.1  W / LSB
//   addr 5     int16   temperature  0.1  C / LSB

namespace {

constexpr int kRxPin = 5;
constexpr int kTxPin = 18;
constexpr uint8_t kSlaveId = 1;
constexpr uint32_t kBaud = 9600;

ModbusRTU s_mb;
HardwareSerial s_serial(1);
uint32_t s_bootMs = 0;
uint32_t s_lastTick = 0;

}  // namespace

namespace WokwiSlave {

void begin() {
    s_serial.begin(kBaud, SERIAL_8N1, kRxPin, kTxPin);
    s_mb.begin(&s_serial);
    s_mb.slave(kSlaveId);
    s_mb.addIreg(0, 0, 6);
    s_bootMs = millis();
    Serial.printf("[WokwiSlave] virtual PZEM on UART1  RX=%d TX=%d  id=%u\n",
                  kRxPin, kTxPin, (unsigned)kSlaveId);
}

void task() {
    s_mb.task();

    if (millis() - s_lastTick > 1000) {
        s_lastTick = millis();
        float t = (millis() - s_bootMs) / 1000.0f;

        uint16_t voltage = (uint16_t)roundf((230.0f + 2.5f * sinf(t / 7.0f)) * 10.0f);
        uint32_t current_mA = (uint32_t)roundf(2300.0f + 400.0f * sinf(t / 11.0f));
        uint32_t power_dW = (uint32_t)roundf((float)voltage * current_mA / 10000.0f * 10.0f);
        int16_t temp_dC = (int16_t)roundf((23.0f + 3.0f * sinf(t / 60.0f)) * 10.0f);

        s_mb.Ireg(0, voltage);
        s_mb.Ireg(1, (uint16_t)((current_mA >> 16) & 0xFFFF));
        s_mb.Ireg(2, (uint16_t)(current_mA & 0xFFFF));
        s_mb.Ireg(3, (uint16_t)((power_dW >> 16) & 0xFFFF));
        s_mb.Ireg(4, (uint16_t)(power_dW & 0xFFFF));
        s_mb.Ireg(5, (uint16_t)temp_dC);
    }
}

}  // namespace WokwiSlave

#endif  // WOKWI_BUILD
