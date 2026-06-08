#pragma once

// Virtual Modbus RTU slave that runs alongside the master in the same
// ESP32 firmware. Used only in Wokwi simulation (gated by WOKWI_BUILD in
// platformio.ini), because the Wokwi Community license does not support
// loading a second board with its own firmware. We work around that by
// running the slave on UART1 on the same chip and cross-wiring it to the
// master's UART2 externally on the diagram.
//
// Real-hardware builds (-e esp32dev) leave these stubs out entirely.

#ifdef WOKWI_BUILD

namespace WokwiSlave {
void begin();
void task();
}  // namespace WokwiSlave

#endif
