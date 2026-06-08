#include "mqtt_publisher.h"

#include <ArduinoJson.h>

static constexpr uint32_t MAX_BACKOFF_MS = 60000;

void MqttPublisher::begin(const GatewayConfig& cfg) {
    m_cfg = &cfg;
    m_client.setServer(cfg.mqttHost.c_str(), cfg.mqttPort);
    m_client.setBufferSize(512);
}

bool MqttPublisher::reconnect() {
    if (!m_cfg) return false;
    Serial.printf("[MQTT] connecting %s:%u\n", m_cfg->mqttHost.c_str(), m_cfg->mqttPort);
    bool ok;
    if (m_cfg->mqttUser.length()) {
        ok = m_client.connect(m_cfg->deviceId.c_str(),
                              m_cfg->mqttUser.c_str(),
                              m_cfg->mqttPass.c_str());
    } else {
        ok = m_client.connect(m_cfg->deviceId.c_str());
    }
    if (ok) {
        m_backoffMs = 1000;
        String onlineTopic = m_cfg->topicPrefix + "/" + m_cfg->deviceId + "/status";
        m_client.publish(onlineTopic.c_str(), "online", true);
        Serial.println("[MQTT] connected");
    } else {
        Serial.printf("[MQTT] failed rc=%d, backoff=%ums\n", m_client.state(), m_backoffMs);
    }
    return ok;
}

void MqttPublisher::loop() {
    if (!m_cfg) return;
    if (m_client.connected()) {
        m_client.loop();
        return;
    }
    uint32_t now = millis();
    if (now - m_lastReconnectAttempt < m_backoffMs) return;
    m_lastReconnectAttempt = now;
    if (!reconnect()) {
        m_backoffMs = min(m_backoffMs * 2, MAX_BACKOFF_MS);
    }
}

bool MqttPublisher::isConnected() {
    return m_client.connected();
}

void MqttPublisher::publishReading(const ModbusReading& r) {
    if (!m_cfg || !m_client.connected()) return;
    JsonDocument doc;
    doc["value"] = r.value;
    doc["unit"] = r.unit;
    doc["ts"] = r.timestampMs;
    doc["slave"] = r.slaveId;
    doc["addr"] = r.address;

    char payload[256];
    size_t n = serializeJson(doc, payload, sizeof(payload));

    String topic = m_cfg->topicPrefix + "/" + m_cfg->deviceId + "/" + r.tag;
    m_client.publish(topic.c_str(), (const uint8_t*)payload, n, false);
}
