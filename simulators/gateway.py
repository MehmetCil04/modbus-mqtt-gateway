"""
Gateway simulator: Python twin of the ESP32 firmware.

Polls a Modbus TCP slave, decodes the registers, and publishes each tag as
JSON to MQTT. The topic/payload schema matches what the real ESP32 firmware
publishes, so the Telegraf/InfluxDB/Grafana pipeline does not need to know
which side produced the data.

Drop-in replacement for the ESP32 firmware during hardware-free testing.
"""
import asyncio
import json
import logging
import os
import struct
import time
from dataclasses import dataclass
from typing import List

import paho.mqtt.client as mqtt
from pymodbus.client import AsyncModbusTcpClient

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
log = logging.getLogger("gateway-sim")

MODBUS_HOST = os.environ.get("MODBUS_HOST", "modbus-slave")
MODBUS_PORT = int(os.environ.get("MODBUS_PORT", "5020"))
MQTT_HOST = os.environ.get("MQTT_HOST", "mosquitto")
MQTT_PORT = int(os.environ.get("MQTT_PORT", "1883"))
DEVICE_ID = os.environ.get("DEVICE_ID", "gw-sim")
TOPIC_PREFIX = os.environ.get("TOPIC_PREFIX", "gateway")
SLAVE_ID = int(os.environ.get("SLAVE_ID", "1"))


@dataclass
class Tag:
    addr: int
    length: int
    dtype: str    # uint16 | int16 | uint32 | float32
    scale: float
    name: str
    unit: str
    poll_s: float


# Mirrors firmware default config (see firmware/src/config_store.cpp::loadDefaults).
TAGS: List[Tag] = [
    Tag(0, 1, "uint16", 0.1, "voltage", "V", 2.0),
    Tag(1, 2, "uint32", 0.001, "current", "A", 2.0),
    Tag(3, 2, "uint32", 0.1, "power", "W", 2.0),
    Tag(5, 1, "int16", 0.1, "temperature", "C", 5.0),
]


def decode(regs: List[int], dtype: str, scale: float) -> float:
    if dtype == "uint16":
        return regs[0] * scale
    if dtype == "int16":
        v = regs[0]
        if v & 0x8000:
            v -= 0x10000
        return v * scale
    if dtype == "uint32":
        return ((regs[0] << 16) | regs[1]) * scale
    if dtype == "float32":
        b = struct.pack(">HH", regs[0], regs[1])
        return struct.unpack(">f", b)[0] * scale
    raise ValueError(f"unknown dtype {dtype}")


async def wait_for_modbus(client: AsyncModbusTcpClient) -> None:
    delay = 1.0
    while True:
        try:
            ok = await client.connect()
            if ok and client.connected:
                log.info("Modbus connected to %s:%d", MODBUS_HOST, MODBUS_PORT)
                return
        except Exception as e:
            log.warning("Modbus connect failed: %s", e)
        await asyncio.sleep(delay)
        delay = min(delay * 2, 30.0)


def make_mqtt() -> mqtt.Client:
    client = mqtt.Client(
        client_id=DEVICE_ID,
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
    )
    status_topic = f"{TOPIC_PREFIX}/{DEVICE_ID}/status"
    client.will_set(status_topic, "offline", retain=True)

    def on_connect(c, userdata, flags, reason_code, properties):
        log.info("MQTT connected (rc=%s)", reason_code)
        c.publish(status_topic, "online", retain=True)

    def on_disconnect(c, userdata, flags, reason_code, properties):
        log.warning("MQTT disconnected (rc=%s)", reason_code)

    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    return client


async def mqtt_connect(client: mqtt.Client) -> None:
    delay = 1.0
    while True:
        try:
            client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
            client.loop_start()
            return
        except Exception as e:
            log.warning("MQTT connect failed: %s", e)
            await asyncio.sleep(delay)
            delay = min(delay * 2, 30.0)


async def poll_tag(modbus: AsyncModbusTcpClient, mqttc: mqtt.Client, tag: Tag) -> None:
    while True:
        try:
            rr = await modbus.read_input_registers(tag.addr, count=tag.length, slave=SLAVE_ID)
            if rr.isError():
                log.warning("modbus error %s: %s", tag.name, rr)
            else:
                value = decode(rr.registers, tag.dtype, tag.scale)
                payload = json.dumps({
                    "value": round(value, 3),
                    "unit": tag.unit,
                    "ts": int(time.time()),
                    "slave": SLAVE_ID,
                    "addr": tag.addr,
                })
                topic = f"{TOPIC_PREFIX}/{DEVICE_ID}/{tag.name}"
                mqttc.publish(topic, payload)
                log.info("%-12s -> %s", tag.name, payload)
        except Exception as e:
            log.exception("poll error %s: %s", tag.name, e)
        await asyncio.sleep(tag.poll_s)


async def main() -> None:
    log.info("Modbus -> tcp://%s:%d", MODBUS_HOST, MODBUS_PORT)
    log.info("MQTT   -> tcp://%s:%d", MQTT_HOST, MQTT_PORT)
    log.info("Device id: %s, prefix: %s", DEVICE_ID, TOPIC_PREFIX)

    modbus = AsyncModbusTcpClient(MODBUS_HOST, port=MODBUS_PORT)
    mqttc = make_mqtt()

    await asyncio.gather(mqtt_connect(mqttc), wait_for_modbus(modbus))

    tasks = [asyncio.create_task(poll_tag(modbus, mqttc, t)) for t in TAGS]
    await asyncio.gather(*tasks)


if __name__ == "__main__":
    asyncio.run(main())
