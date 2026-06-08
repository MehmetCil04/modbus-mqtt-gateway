#pragma once

#include <Arduino.h>
#include <vector>

struct ModbusTag {
    uint8_t slaveId;        // Modbus slave address (1-247)
    uint8_t functionCode;   // 3 = holding, 4 = input
    uint16_t address;       // register address
    uint8_t length;         // number of registers (1 or 2 for 32-bit)
    String dataType;        // "uint16", "int16", "uint32", "float32"
    float scale;            // multiplier
    String tag;             // MQTT tag (e.g. "voltage")
    String unit;            // "V", "A", "C", ...
    uint32_t pollIntervalMs;
};

struct GatewayConfig {
    // Modbus serial
    int uartRx = 16;
    int uartTx = 17;
    int dePin = 4;          // RS485 driver enable
    uint32_t baud = 9600;
    uint8_t serialConfig = 0; // SERIAL_8N1 mapped at use site

    // MQTT
    String mqttHost = "192.168.1.10";
    uint16_t mqttPort = 1883;
    String mqttUser;
    String mqttPass;
    String deviceId = "gw-01";
    String topicPrefix = "gateway";

    std::vector<ModbusTag> tags;

    bool load();
    bool save() const;
    void loadDefaults();
};
