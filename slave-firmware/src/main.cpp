// Modbus RTU slave firmware — used in the Wokwi multi-board simulation as a
// stand-in for a PZEM-004T / SHT20 style industrial meter on the RS485 bus.
//
// Register map (input registers, FC=04), identical to what the Python
// modbus_slave.py simulator exposes, so the gateway can talk to either
// without changing its config:
//   addr 0     uint16  voltage      0.1  V / LSB
//   addr 1-2   uint32  current      0.001 A / LSB   (big-endian word order)
//   addr 3-4   uint32  power        0.1  W / LSB
//   addr 5     int16   temperature  0.1  C / LSB
//
// UART2 on GPIO16 (RX) and GPIO17 (TX), 9600 8N1.
// Wokwi wiring (crossed):
//   master TX (esp:17) -> slave RX (slave:16)
//   master RX (esp:16) <- slave TX (slave:17)
//   common GND
// In real hardware these two lines would go through a MAX485 each and meet on
// the RS485 differential pair (A/B). The simulation collapses that into two
// direct UART wires.

#include <Arduino.h>
#include <ModbusRTU.h>

constexpr int RX_PIN = 16;
constexpr int TX_PIN = 17;
constexpr uint8_t SLAVE_ID = 1;
constexpr uint32_t BAUD = 9600;

ModbusRTU mb;
HardwareSerial mbSerial(2);

uint32_t s_lastUpdate = 0;
uint32_t s_bootMs = 0;

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[Slave] PZEM-like Modbus RTU slave starting");

    mbSerial.begin(BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
    mb.begin(&mbSerial);
    mb.slave(SLAVE_ID);

    // 6 input registers starting at address 0
    mb.addIreg(0, 0, 6);

    s_bootMs = millis();
    Serial.printf("[Slave] id=%u baud=%lu rx=%d tx=%d\n",
                  (unsigned)SLAVE_ID, (unsigned long)BAUD, RX_PIN, TX_PIN);
}

void loop() {
    mb.task();

    if (millis() - s_lastUpdate > 1000) {
        s_lastUpdate = millis();
        float t = (millis() - s_bootMs) / 1000.0f;

        uint16_t voltage = (uint16_t)round((230.0f + 2.5f * sinf(t / 7.0f)) * 10.0f);
        uint32_t current_mA = (uint32_t)round(2300.0f + 400.0f * sinf(t / 11.0f));
        uint32_t power_dW = (uint32_t)round((float)voltage * current_mA / 10000.0f * 10.0f);
        int16_t temp_dC = (int16_t)round((23.0f + 3.0f * sinf(t / 60.0f)) * 10.0f);

        mb.Ireg(0, voltage);
        mb.Ireg(1, (uint16_t)((current_mA >> 16) & 0xFFFF));
        mb.Ireg(2, (uint16_t)(current_mA & 0xFFFF));
        mb.Ireg(3, (uint16_t)((power_dW >> 16) & 0xFFFF));
        mb.Ireg(4, (uint16_t)(power_dW & 0xFFFF));
        mb.Ireg(5, (uint16_t)temp_dC);

        Serial.printf("V=%5.1fV  I=%6.3fA  P=%6.1fW  T=%5.1fC\n",
                      voltage / 10.0f,
                      current_mA / 1000.0f,
                      power_dW / 10.0f,
                      temp_dC / 10.0f);
    }

    delay(1);
}
