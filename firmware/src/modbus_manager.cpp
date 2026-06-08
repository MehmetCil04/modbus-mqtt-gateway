#include "modbus_manager.h"

#include <ModbusRTU.h>

#include "wokwi_slave.h"

namespace {
ModbusRTU s_mb;
HardwareSerial s_serial(2);
volatile bool s_txDone = false;

bool cb(Modbus::ResultCode code, uint16_t, void*) {
    s_txDone = true;
    return true;
}
}  // namespace

void ModbusManager::begin(const GatewayConfig& cfg) {
    m_cfg = &cfg;
    s_serial.begin(cfg.baud, SERIAL_8N1, cfg.uartRx, cfg.uartTx);
    s_mb.begin(&s_serial, cfg.dePin);
    s_mb.master();
    m_lastPollMs.assign(cfg.tags.size(), 0);
}

float ModbusManager::decode(const ModbusTag& tag, const uint16_t* regs) const {
    if (tag.dataType == "int16") {
        return (int16_t)regs[0] * tag.scale;
    } else if (tag.dataType == "uint32") {
        uint32_t v = ((uint32_t)regs[0] << 16) | regs[1];
        return v * tag.scale;
    } else if (tag.dataType == "float32") {
        uint32_t v = ((uint32_t)regs[0] << 16) | regs[1];
        float f;
        memcpy(&f, &v, sizeof(f));
        return f * tag.scale;
    }
    // uint16 default
    return regs[0] * tag.scale;
}

bool ModbusManager::readTag(const ModbusTag& tag, float& outValue) {
    static uint16_t buf[4];
    s_txDone = false;

    uint16_t res = (tag.functionCode == 3)
                       ? s_mb.readHreg(tag.slaveId, tag.address, buf, tag.length, cb)
                       : s_mb.readIreg(tag.slaveId, tag.address, buf, tag.length, cb);
    if (!res) return false;

    uint32_t start = millis();
    while (s_mb.slave() && millis() - start < 1000) {
        s_mb.task();
#ifdef WOKWI_BUILD
        // In Wokwi simulation the virtual slave runs on the same chip.
        // Pump it during this busy-wait so it can actually answer the
        // request we just sent — otherwise we'd time out before the
        // slave's task() got any CPU time.
        WokwiSlave::task();
#endif
        delay(1);
    }
    if (!s_txDone) return false;

    outValue = decode(tag, buf);
    return true;
}

void ModbusManager::poll(ReadingCallback cb_) {
    if (!m_cfg) return;
    uint32_t now = millis();

    for (size_t i = 0; i < m_cfg->tags.size(); ++i) {
        const auto& tag = m_cfg->tags[i];
        if (now - m_lastPollMs[i] < tag.pollIntervalMs) continue;
        m_lastPollMs[i] = now;

        float v = 0.0f;
        if (readTag(tag, v)) {
            ++m_successCount;
            ModbusReading r{tag.tag, tag.unit, v, now, tag.slaveId, tag.address};
            cb_(r);
        } else {
            ++m_errorCount;
        }
    }
}
