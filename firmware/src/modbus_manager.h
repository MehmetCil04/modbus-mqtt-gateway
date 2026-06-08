#pragma once

#include <Arduino.h>
#include <functional>

#include "config_store.h"

struct ModbusReading {
    String tag;
    String unit;
    float value;
    uint32_t timestampMs;
    uint8_t slaveId;
    uint16_t address;
};

class ModbusManager {
   public:
    using ReadingCallback = std::function<void(const ModbusReading&)>;

    void begin(const GatewayConfig& cfg);
    void poll(ReadingCallback cb);

    uint32_t successCount() const { return m_successCount; }
    uint32_t errorCount() const { return m_errorCount; }

   private:
    const GatewayConfig* m_cfg = nullptr;
    std::vector<uint32_t> m_lastPollMs;
    uint32_t m_successCount = 0;
    uint32_t m_errorCount = 0;

    bool readTag(const ModbusTag& tag, float& outValue);
    float decode(const ModbusTag& tag, const uint16_t* regs) const;
};
