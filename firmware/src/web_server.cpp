#include "web_server.h"

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Update.h>

namespace {
AsyncWebServer s_server(80);
}

void ConfigWebServer::begin() {
    LittleFS.begin(true);

    s_server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    s_server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* req) {
        JsonDocument doc;
        doc["deviceId"] = m_cfg->deviceId;
        doc["mqttHost"] = m_cfg->mqttHost;
        doc["mqttPort"] = m_cfg->mqttPort;
        doc["mqttUser"] = m_cfg->mqttUser;
        doc["topicPrefix"] = m_cfg->topicPrefix;
        doc["baud"] = m_cfg->baud;
        JsonArray arr = doc["tags"].to<JsonArray>();
        for (const auto& t : m_cfg->tags) {
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
        String out;
        serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    auto* handler = new AsyncCallbackJsonWebHandler(
        "/api/config",
        [this](AsyncWebServerRequest* req, JsonVariant& json) {
            JsonObject body = json.as<JsonObject>();
            m_cfg->deviceId = String((const char*)(body["deviceId"] | m_cfg->deviceId.c_str()));
            m_cfg->mqttHost = String((const char*)(body["mqttHost"] | m_cfg->mqttHost.c_str()));
            m_cfg->mqttPort = body["mqttPort"] | m_cfg->mqttPort;
            m_cfg->mqttUser = String((const char*)(body["mqttUser"] | ""));
            m_cfg->mqttPass = String((const char*)(body["mqttPass"] | ""));
            m_cfg->topicPrefix = String((const char*)(body["topicPrefix"] | m_cfg->topicPrefix.c_str()));
            m_cfg->baud = body["baud"] | m_cfg->baud;

            m_cfg->tags.clear();
            for (JsonObject t : body["tags"].as<JsonArray>()) {
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
                m_cfg->tags.push_back(tag);
            }
            bool ok = m_cfg->save();
            req->send(ok ? 200 : 500, "application/json",
                      ok ? "{\"ok\":true}" : "{\"ok\":false}");
        });
    s_server.addHandler(handler);

    s_server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        JsonDocument doc;
        doc["fw"] = FIRMWARE_VERSION;
        doc["uptimeMs"] = millis();
        doc["okCount"] = m_modbus->successCount();
        doc["errCount"] = m_modbus->errorCount();
        String out;
        serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    s_server.on(
        "/api/ota", HTTP_POST,
        [](AsyncWebServerRequest* req) {
            bool ok = !Update.hasError();
            AsyncWebServerResponse* res = req->beginResponse(
                ok ? 200 : 500, "text/plain", ok ? "OK" : "FAIL");
            res->addHeader("Connection", "close");
            req->send(res);
            if (ok) {
                delay(500);
                ESP.restart();
            }
        },
        [](AsyncWebServerRequest* req, String filename, size_t index,
           uint8_t* data, size_t len, bool final) {
            if (!index) {
                Serial.printf("[OTA] upload %s\n", filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
            }
            if (Update.write(data, len) != len) Update.printError(Serial);
            if (final) {
                if (Update.end(true)) {
                    Serial.printf("[OTA] %u bytes flashed\n", index + len);
                } else {
                    Update.printError(Serial);
                }
            }
        });

    s_server.begin();
}
