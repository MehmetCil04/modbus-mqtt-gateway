# Endüstriyel IoT Gateway — Modbus RTU → MQTT → Bulut

ESP32 tabanlı, RS485 üzerinden Modbus RTU sensör/sayaç okuyup MQTT ile buluta gönderen ve
InfluxDB + Grafana üzerinde canlı görselleştiren endüstriyel IoT gateway. Yapılandırma
tarayıcıdan yapılır, firmware OTA ile güncellenir.

![Grafana dashboard](docs/grafana-dashboard.png)
*Canlı dashboard: gerilim, akım, güç ve sıcaklık zaman serileri + anlık ölçüm kartları.*

```
[Modbus Slave] --RS485--> [ESP32 Gateway] --WiFi/MQTT--> [Mosquitto] --> [Telegraf] --> [InfluxDB] --> [Grafana]
                              |       \
                          OLED durum   Web UI (config + OTA)
```

Donanım hâlâ yolda mı? Sorun değil — proje **iki farklı donanımsız test modu** ile birlikte gelir.

<p align="center">
  <img src="docs/wokwi-simulation.png" alt="Wokwi simulation" width="500">
  <br>
  <em>Aynı firmware Wokwi'de sanal ESP32 üzerinde çalışıyor. WiFi + MQTT bağlantısı OK; gerçek bir Modbus slave olmadığı için ERR sayacı artıyor (beklenen davranış).</em>
</p>

## Hızlı Başlangıç (Donanımsız)

İhtiyacınız olan tek şey **Docker Desktop**:

```bash
cd cloud
docker compose up -d --build
```

Bu komut **6 servisi** ayağa kaldırır:

| Servis | Açıklama |
|---|---|
| `mosquitto` | MQTT broker |
| `telegraf` | MQTT → InfluxDB köprüsü |
| `influxdb` | Zaman serisi veritabanı |
| `grafana` | Canlı dashboard |
| `modbus-slave` | **Sahte Modbus TCP sayaç** (V/A/W/°C) — Python |
| `gateway-sim` | **ESP32 firmware'inin yazılım eşi** — Python, gerçek Modbus + MQTT konuşur |

Ardından:

- **Grafana:** http://localhost:3000 (admin / admin) — `Gateway Telemetri` dashboard'u otomatik yüklenir
- **InfluxDB UI:** http://localhost:8086 (admin / changeme123)
- **MQTT broker:** `localhost:1883`

20-30 saniye içinde dashboard'da V/A/W/°C için canlı çizgi grafikleri görmelisiniz.

## Üç Çalıştırma Yöntemi

| Yöntem | Donanım | Kurulum | Ne yapar |
|---|---|---|---|
| **Python tam stack** | Yok | Docker Desktop | Tam pipeline'ı simüle eder. Gerçek Modbus TCP + MQTT protokolleri. CV demosu için ideal. |
| **Wokwi** | Yok | VS Code + Wokwi ext. | Gerçek ESP32 firmware'i sanal donanımda. WiFi + OLED'i tarayıcıda görürsünüz. |
| **Gerçek donanım** | ESP32 + MAX485 + Modbus sensör | USB | Üretim modu. PCB veya breadboard ile gerçek saha kullanımı. |

## Wokwi Simülasyonu (Tarayıcıda Sanal ESP32 — Multi-Board)

Wokwi yapılandırması **iki ESP32**'yi yan yana çalıştırır: master gateway'imiz ve
ayrı bir slave firmware'i (`slave-firmware/`) ile çalışan sahte PZEM-004T enerji
sayacı. UART hatları çapraz bağlanır — gerçek RS485 kablolaması simülasyonda
şeffaf hale gelir (MAX485 IC'leri atlanır çünkü yazılım katmanı değişmez).

```
┌──────────────────┐    GPIO17 (TX)    ┌──────────────────┐
│  Gateway ESP32   │ ───────────────▶ │   Slave ESP32    │
│  - Modbus master │    GPIO16 (RX)    │   - Modbus slave │
│  - MQTT publish  │ ◀─────────────── │   - PZEM register │
│  - OLED + WebUI  │       GND         │     map (V/I/P/T) │
└──────────────────┘ ◀──────────────▶ └──────────────────┘
```

### Kurulum

1. VS Code'a [Wokwi Simulator eklentisi](https://marketplace.visualstudio.com/items?itemName=wokwi.wokwi-vscode) kurun
2. Eklenti talep ettiğinde wokwi.com'da ücretsiz lisans alın (tek tıklama)
3. **Her iki firmware'i de derleyin:**
   ```bash
   # Gateway (master)
   cd firmware && pio run -e wokwi

   # Modbus slave (PZEM-like)
   cd ../slave-firmware && pio run
   ```
4. VS Code'da `firmware/` klasörünü açın, `Ctrl+Shift+P` → **"Wokwi: Start Simulator"**

Wokwi diyagramı `slave-firmware/.pio/build/wokwi-slave/firmware.bin`'i otomatik
olarak ikinci ESP32'ye yükler. Her iki cihaz da gerçek Modbus RTU üzerinden
konuşur, gateway okumayı `test.mosquitto.org` MQTT broker'ına yayınlar.

### Beklenen Çıktı

| OLED satırı | Anlamı |
|---|---|
| `WiFi: OK  MQTT: OK` | Wokwi-GUEST'e ve public broker'a bağlandı |
| `IP: 10.13.37.x` | Wokwi'nin tahsis ettiği DHCP IP'si |
| `OK: 60+` (artan) | Slave'den başarılı Modbus okumaları |
| `ERR: 0` | Slave bağlıyken hata olmaz |

Public broker'dan okumaları doğrulamak için:
```bash
mosquitto_sub -h test.mosquitto.org -t "gateway/gw-01/#" -v
```

## Gerçek Donanım Modu

### Bileşenler

| Bileşen | Açıklama |
|---|---|
| ESP32 DevKit (WROOM-32) | Ana MCU |
| MAX485 / TTL-RS485 modülü | RS485 fiziksel katmanı |
| 0.96" SSD1306 OLED (I2C, 0x3C) | Yerel durum göstergesi |
| Modbus slave cihaz | PZEM-004T v3, SHT20 veya Arduino slave simülatörü |

### Bağlantılar (varsayılan)

| ESP32 | Bağlandığı |
|---|---|
| GPIO 16 | RS485 RO (Receiver Output) |
| GPIO 17 | RS485 DI (Driver Input) |
| GPIO 4 | RS485 DE+RE (kısa devre) |
| GPIO 21 | OLED SDA |
| GPIO 22 | OLED SCL |

### Yükleme

```bash
cd firmware
pio run -e esp32dev -t upload          # firmware
pio run -e esp32dev -t uploadfs        # LittleFS (web UI)
pio device monitor                     # seri çıktıyı izle
```

İlk açılışta ESP32 `modbus-gateway-XXXX` adlı bir WiFi AP açar; bağlanıp ev WiFi bilgilerinizi
girersiniz. Sonra OLED'de gösterilen IP'ye tarayıcıdan bağlanıp MQTT sunucusunu, hangi
Modbus register'larının okunacağını yapılandırırsınız.

## Proje Yapısı

```
modbus-mqtt-gateway/
├── firmware/                    PlatformIO ESP32 firmware (master — gateway)
│   ├── src/                     Modüler kaynaklar (modbus, mqtt, config, web, OTA, OLED)
│   ├── data/                    LittleFS — web UI HTML/JS
│   ├── platformio.ini           [env:esp32dev] ve [env:wokwi] hedefleri
│   ├── wokwi.toml + diagram.json Wokwi multi-board konfigürasyonu
├── slave-firmware/              ESP32 Modbus RTU slave (PZEM benzeri sayaç)
│   ├── platformio.ini           [env:wokwi-slave]
│   └── src/main.cpp             Input register'lar + sinüzoidal değerler
├── simulators/                  Donanımsız test için Python simülatörleri
│   ├── modbus_slave.py          Sahte enerji sayacı (Modbus TCP)
│   ├── gateway.py               ESP32 firmware'inin Python eşi
│   └── Dockerfile
├── cloud/                       Docker Compose bulut yığını
│   ├── docker-compose.yml       6 servis: broker + telegraf + InfluxDB + Grafana + simülatörler
│   ├── mosquitto.conf / telegraf.conf
│   └── grafana-provisioning/    Datasource + dashboard otomatik provisioning
└── docs/                        Mimari notları ve ekran görüntüleri
```

## Teknolojiler

**Firmware:** C++, ESP32 Arduino Core, FreeRTOS, eMobus, PubSubClient, ESPAsyncWebServer (ESP32Async fork), LittleFS, Adafruit SSD1306, WiFiManager, ArduinoOTA  
**Simülatörler:** Python 3.12, asyncio, PyModbus, paho-mqtt  
**Bulut yığını:** Docker Compose, Eclipse Mosquitto, Telegraf, InfluxDB 2.x, Grafana 11.x  
**Tooling:** PlatformIO, Wokwi

## Yol Haritası

- [x] Modbus RTU master + JSON MQTT publish
- [x] Web tabanlı konfigürasyon + OTA güncelleme
- [x] InfluxDB + Grafana ile zaman serisi görselleştirme
- [x] Python tabanlı tam stack simülatör (donanımsız test)
- [x] Wokwi entegrasyonu (tarayıcıda sanal ESP32)
- [ ] TLS destekli MQTT (HiveMQ Cloud)
- [ ] Çevrimdışı veri buffer'ı (LittleFS ring buffer)
- [ ] Modbus TCP master desteği
- [ ] İmzalı OTA firmware

## Lisans

MIT
