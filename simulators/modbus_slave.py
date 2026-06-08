"""
Modbus TCP slave simulator.

Mimics a 3-phase industrial energy meter (e.g., PZEM-016).
Exposes input registers (function code 4):
  addr 0     uint16  voltage      scale 0.1  -> 2305 = 230.5 V
  addr 1-2   uint32  current_mA   scale 0.001 -> 2340 = 2.340 A
  addr 3-4   uint32  power_dW     scale 0.1  -> 5400 = 540.0 W
  addr 5     int16   temp_dC      scale 0.1  -> 235 = 23.5 C

Values fluctuate sinusoidally with small random noise so dashboards look alive.
"""
import asyncio
import logging
import math
import os
import random
import time

from pymodbus.datastore import (
    ModbusSequentialDataBlock,
    ModbusServerContext,
    ModbusSlaveContext,
)
from pymodbus.server import StartAsyncTcpServer

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
log = logging.getLogger("modbus-slave")

HOST = os.environ.get("SLAVE_HOST", "0.0.0.0")
PORT = int(os.environ.get("SLAVE_PORT", "5020"))
SLAVE_ID = int(os.environ.get("SLAVE_ID", "1"))
TICK_S = float(os.environ.get("TICK_S", "1.0"))


class SimMeter:
    def __init__(self, context: ModbusServerContext) -> None:
        self.context = context
        self.t0 = time.time()

    def tick(self) -> None:
        t = time.time() - self.t0
        voltage = round((230 + 2.5 * math.sin(t / 7) + random.uniform(-0.5, 0.5)) * 10)
        current_mA = max(0, round(2300 + 400 * math.sin(t / 11) + random.uniform(-50, 50)))
        # P = V * I (approx); voltage is in 0.1V, current in mA => W*10
        power_dW = round(voltage * current_mA / 10000 * 10)
        temp_dC = round((23 + 3 * math.sin(t / 60) + random.uniform(-0.2, 0.2)) * 10)

        regs = [
            voltage & 0xFFFF,
            (current_mA >> 16) & 0xFFFF,
            current_mA & 0xFFFF,
            (power_dW >> 16) & 0xFFFF,
            power_dW & 0xFFFF,
            temp_dC & 0xFFFF,
        ]
        # fc=4 == input registers
        self.context[SLAVE_ID].setValues(4, 0, regs)
        log.info(
            "V=%5.1fV  I=%6.3fA  P=%6.1fW  T=%4.1fC",
            voltage / 10, current_mA / 1000, power_dW / 10, temp_dC / 10,
        )


async def updater(meter: SimMeter) -> None:
    while True:
        meter.tick()
        await asyncio.sleep(TICK_S)


async def main() -> None:
    store = ModbusSlaveContext(
        ir=ModbusSequentialDataBlock(0, [0] * 100),
        hr=ModbusSequentialDataBlock(0, [0] * 100),
    )
    context = ModbusServerContext(slaves={SLAVE_ID: store}, single=False)
    meter = SimMeter(context)
    meter.tick()  # populate initial values

    log.info("Modbus TCP slave on %s:%d (slave id %d)", HOST, PORT, SLAVE_ID)
    await asyncio.gather(
        StartAsyncTcpServer(context=context, address=(HOST, PORT)),
        updater(meter),
    )


if __name__ == "__main__":
    asyncio.run(main())
