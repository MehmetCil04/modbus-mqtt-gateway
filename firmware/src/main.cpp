#include <Arduino.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

#include "config_store.h"
#include "display.h"
#include "modbus_manager.h"
#include "mqtt_publisher.h"
#include "ota_updater.h"
#include "web_server.h"

static constexpr uint32_t WDT_TIMEOUT_S = 30;
static constexpr uint32_t LOOP_INTERVAL_MS = 100;

GatewayConfig g_config;
ModbusManager g_modbus;
MqttPublisher g_mqtt;
DisplayUI g_display;
ConfigWebServer g_web(&g_config, &g_modbus);

static uint32_t s_lastStatusUpdate = 0;

static void connectWiFi() {
    g_display.showMessage("WiFi", "baglantisi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin();
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
        delay(250);
    }
    if (WiFi.status() != WL_CONNECTED) {
        g_display.showMessage("WiFi", "AP modu");
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[Gateway] booting v" FIRMWARE_VERSION);

    esp_task_wdt_init(WDT_TIMEOUT_S, true);
    esp_task_wdt_add(NULL);

    g_display.begin();
    g_display.showMessage("Gateway", "v" FIRMWARE_VERSION);

    if (!g_config.load()) {
        Serial.println("[Config] yuklenemedi, varsayilanlar kullaniliyor");
        g_config.loadDefaults();
    }

    g_modbus.begin(g_config);
    connectWiFi();

    if (WiFi.status() == WL_CONNECTED) {
        g_mqtt.begin(g_config);
        OtaUpdater::begin();
    }
    g_web.begin();
}

void loop() {
    esp_task_wdt_reset();

    g_modbus.poll([](const ModbusReading& r) {
        g_mqtt.publishReading(r);
    });

    g_mqtt.loop();
    OtaUpdater::loop();

    if (millis() - s_lastStatusUpdate > 1000) {
        s_lastStatusUpdate = millis();
        g_display.showStatus(
            WiFi.status() == WL_CONNECTED,
            g_mqtt.isConnected(),
            g_modbus.successCount(),
            g_modbus.errorCount());
    }

    delay(LOOP_INTERVAL_MS);
}
