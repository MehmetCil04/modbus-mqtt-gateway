#pragma once

#include "config_store.h"
#include "modbus_manager.h"

class ConfigWebServer {
   public:
    ConfigWebServer(GatewayConfig* cfg, ModbusManager* modbus)
        : m_cfg(cfg), m_modbus(modbus) {}
    void begin();

   private:
    GatewayConfig* m_cfg;
    ModbusManager* m_modbus;
};
