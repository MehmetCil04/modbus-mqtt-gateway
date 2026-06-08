#include "config_store.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

static constexpr const char* kConfigPath = "/config.json";

void GatewayConfig::loadDefaults() {
    tags.clear();
    ModbusTag voltage{1, 4, 0x0000, 1, "uint16", 0.1f, "voltage", "V", 2000};
    ModbusTag current{1, 4, 0x0001, 2, "uint32", 0.001f, "current", "A", 2000};
    tags.push_back(voltage);
    tags.push_back(current);
}

bool GatewayConfig::load() {
    if (!LittleFS.begin(true)) return false;
    if (!LittleFS.exists(kConfigPath)) return false;

    File f = LittleFS.open(kConfigPath, "r");
    if (!f) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) return false;

    uartRx = doc["uartRx"] | uartRx;
    uartTx = doc["uartTx"] | uartTx;
    dePin = doc["dePin"] | dePin;
    baud = doc["baud"] | baud;

    mqttHost = String((const char*)(doc["mqttHost"] | mqttHost.c_str()));
    mqttPort = doc["mqttPort"] | mqttPort;
    mqttUser = String((const char*)(doc["mqttUser"] | ""));
    mqttPass = String((const char*)(doc["mqttPass"] | ""));
    deviceId = String((const char*)(doc["deviceId"] | deviceId.c_str()));
    topicPrefix = String((const char*)(doc["topicPrefix"] | topicPrefix.c_str()));

    tags.clear();
    for (JsonObject t : doc["tags"].as<JsonArray>()) {
        ModbusTag tag;
        tag.slaveId = t["slaveId"] | 1;
        tag.functionCode = t["fc"] | 4;
        tag.address = t["addr"] | 0;
        tag.length = t["len"] | 1;
        tag.dataType = String((const char*)(t["type"] | "uint16"));
        tag.scale = t["scale"] | 1.0f;
        tag.tag = String((const char*)(t["tag"] | "tag"));
        tag.unit = String((const char*)(t["unit"] | ""));
        tag.pollIntervalMs = t["pollMs"] | 2000;
        tags.push_back(tag);
    }
    return true;
}

bool GatewayConfig::save() const {
    if (!LittleFS.begin(true)) return false;
    File f = LittleFS.open(kConfigPath, "w");
    if (!f) return false;

    JsonDocument doc;
    doc["uartRx"] = uartRx;
    doc["uartTx"] = uartTx;
    doc["dePin"] = dePin;
    doc["baud"] = baud;
    doc["mqttHost"] = mqttHost;
    doc["mqttPort"] = mqttPort;
    doc["mqttUser"] = mqttUser;
    doc["mqttPass"] = mqttPass;
    doc["deviceId"] = deviceId;
    doc["topicPrefix"] = topicPrefix;

    JsonArray arr = doc["tags"].to<JsonArray>();
    for (const auto& t : tags) {
        JsonObject o = arr.add<JsonObject>();
        o["slaveId"] = t.slaveId;
        o["fc"] = t.functionCode;
        o["addr"] = t.address;
        o["len"] = t.length;
        o["type"] = t.dataType;
        o["scale"] = t.scale;
        o["tag"] = t.tag;
        o["unit"] = t.unit;
        o["pollMs"] = t.pollIntervalMs;
    }
    serializeJson(doc, f);
    f.close();
    return true;
}
