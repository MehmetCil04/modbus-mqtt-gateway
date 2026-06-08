#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

#include "config_store.h"
#include "modbus_manager.h"

class MqttPublisher {
   public:
    void begin(const GatewayConfig& cfg);
    void loop();
    bool isConnected();
    void publishReading(const ModbusReading& r);

   private:
    const GatewayConfig* m_cfg = nullptr;
    WiFiClient m_wifi;
    PubSubClient m_client{m_wifi};
    uint32_t m_lastReconnectAttempt = 0;
    uint32_t m_backoffMs = 1000;

    bool reconnect();
};
