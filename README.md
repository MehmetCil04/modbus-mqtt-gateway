# Endüstriyel IoT Gateway — Modbus RTU → MQTT → Bulut

ESP32 tabanlı, RS485 üzerinden Modbus RTU sensör/sayaç okuyup MQTT ile buluta gönderen ve
InfluxDB + Grafana üzerinde canlı görselleştiren bir endüstriyel IoT gateway. Yapılandırma
tarayıcıdan yapılır, firmware OTA ile güncellenir.

```
[Modbus Slave] --RS485--> [ESP32 Gateway] --WiFi/MQTT--> [Mosquitto] --> [Telegraf] --> [InfluxDB] --> [Grafana]
                              |       \
                          OLED durum   Web UI (config + OTA)
```

## Donanım

| Bileşen | Açıklama |
|---|---|
| ESP32 DevKit (WROOM-32) | Ana MCU |
| MAX485 / TTL-RS485 modülü | RS485 fiziksel katmanı |
| 0.96" SSD1306 OLED (I2C, 0x3C) | Yerel durum göstergesi |
| Modbus slave cihaz | PZEM-004T v3, SHT20 veya Arduino slave simülatörü |

**Bağlantılar (varsayılan):**
- RS485 RO → GPIO16 (RX)
- RS485 DI → GPIO17 (TX)
- RS485 DE+RE birleşik → GPIO4
- OLED SDA/SCL → GPIO21/22

## Hızlı Başlangıç

### 1. Bulut yığınını kaldır

```bash
cd cloud
docker compose up -d
```

- MQTT: `localhost:1883`
- InfluxDB UI: `http://localhost:8086` (admin / changeme123)
- Grafana: `http://localhost:3000` (admin / admin) → "Gateway Telemetri" dashboard hazır gelir

### 2. Firmware'i derle ve yükle

```bash
cd firmware
pio run -t upload                   # firmware
pio run -t uploadfs                 # LittleFS (web UI)
pio device monitor                  # seri çıktıyı izle
```

İlk açılışta cihaz `modbus-gateway-XXXX` adlı bir AP açar; bağlanıp WiFi kimlik bilgilerini girin.

### 3. Yapılandırma

Tarayıcıdan ESP32'nin IP'sine bağlanın (OLED ekranda görünür):

- MQTT sunucu adresi, port, kullanıcı/parola
- Hangi Modbus slave'in hangi register'larının okunacağı
- Her tag için: slave id, function code (3/4), adres, uzunluk, veri tipi, ölçek, MQTT tag adı, poll periyodu

### 4. Doğrulama

```bash
# MQTT mesajlarını izle
mosquitto_sub -h localhost -t "gateway/#" -v
```

Grafana'da `Gateway Telemetri` dashboard'unda canlı veri akışını göreceksiniz.

## Modbus Slave Test

Gerçek bir Modbus cihaz yoksa testler için:

- **Windows:** [Modbus Slave](https://www.modbustools.com/modbus_slave.html) (USB-RS485 dönüştürücü)
- **Arduino:** İkinci bir Arduino/ESP32'yi `eModbus` veya `ModbusSlave` kütüphanesiyle slave olarak yapılandırın

## Proje Yapısı

```
modbus-mqtt-gateway/
├── firmware/          PlatformIO ESP32 firmware (Arduino framework)
│   ├── src/           Modüler kaynaklar (modbus, mqtt, config, web, OTA, OLED)
│   └── data/          LittleFS — web UI HTML/JS
├── cloud/             Docker Compose: Mosquitto + Telegraf + InfluxDB + Grafana
└── docs/              Mimari diyagramı, bağlantı şeması, demo
```

## Yol Haritası

- [x] Modbus RTU master + JSON MQTT publish
- [x] Web tabanlı konfigürasyon + OTA
- [x] InfluxDB + Grafana ile zaman serisi görselleştirme
- [ ] TLS destekli MQTT (HiveMQ Cloud)
- [ ] Çevrimdışı veri buffer'ı (LittleFS ring buffer)
- [ ] Modbus TCP master desteği

## Lisans

MIT
