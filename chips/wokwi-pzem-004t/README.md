# wokwi-pzem-004t

A Wokwi custom chip that simulates a [PZEM-004T v3](https://innovatorsguru.com/pzem-004t-v3/) industrial energy meter. Compiles to WebAssembly via clang and drops into any Wokwi diagram as a four-pin module (GND / VCC / RX / TX).

## What it does

- Speaks Modbus RTU on its UART pins (9600 8N1 by default, slave id 1).
- Answers function codes **03** (read holding registers) and **04** (read input registers) — treated identically.
- Returns voltage, current, power, and temperature with sinusoidal variation + light noise so dashboards look alive.
- Generates correct Modbus CRC-16 on every response.
- Sends Modbus exceptions (`0x80 | fc`) for illegal function, address, or value.

## Register map

| Addr | Type    | Value       | Scale       |
|-----:|---------|-------------|-------------|
| 0    | uint16  | Voltage     | 0.1 V / LSB |
| 1–2  | uint32  | Current     | 0.001 A / LSB (big-endian word order) |
| 3–4  | uint32  | Power       | 0.1 W / LSB |
| 5    | int16   | Temperature | 0.1 °C / LSB |

This layout deliberately matches the gateway firmware's default config, so the chip is a drop-in replacement for the on-chip `WokwiSlave` UART loopback used in earlier revisions.

## Build

Requires **clang with wasm32 target** (LLVM 14+).

```bash
# Windows
winget install LLVM.LLVM
# then add C:\Program Files\LLVM\bin to PATH

cd chips/wokwi-pzem-004t
make
```

Output: `chip.wasm`.

## Use in a diagram

```json
{
  "type": "chip-wokwi-pzem-004t",
  "id": "meter",
  "top": 0,
  "left": 200,
  "attrs": {}
}
```

Wire ESP32 UART2 (or any UART) to the chip's RX/TX (crossed):

```
ESP32 TX (GPIO17) --> meter:RX
ESP32 RX (GPIO16) <-- meter:TX
ESP32 GND         --- meter:GND
ESP32 5V          --- meter:VCC
```

The chip ignores VCC level (Wokwi UARTs are logical), but the pin exists so the diagram looks like a real PZEM hookup.

## License

MIT — same as the parent gateway project.
