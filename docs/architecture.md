# Mimari Notları

## Veri Akışı

1. **ESP32 firmware** her tag için `pollIntervalMs` periyoduyla Modbus RTU üzerinden
   ilgili slave'in register'larını okur (`function code 3` = holding, `4` = input).
2. Okunan ham `uint16[]` veri `dataType` ve `scale` ile ölçeklenir (uint16/int16/uint32/float32).
3. Her okuma `gateway/{deviceId}/{tag}` topic'ine JSON olarak yayınlanır:
   `{"value": 230.4, "unit": "V", "ts": 12345678, "slave": 1, "addr": 0}`
4. **Telegraf** Mosquitto'dan abone olur, topic'ten `device` ve `tag` etiketlerini çıkarır,
   InfluxDB'ye `gateway` measurement'ı olarak yazar.
5. **Grafana** Flux sorgularıyla InfluxDB'den okur ve dashboard'da gösterir.

## Hata Toleransı

- **WiFi:** WiFiManager AP fallback. Bağlantı koparsa Arduino çekirdeği otomatik yeniden bağlanır.
- **MQTT:** Üstel backoff (1s → 2s → ... → 60s). Yeniden bağlandığında retain'li `status=online` mesajı.
- **Modbus:** Her okumada 1 saniye timeout, başarısız okuma `errCount`'a yazılır.
- **Watchdog:** 30 saniyelik task watchdog. Donmuş döngü durumunda cihaz reset olur.

## Güvenlik Yol Haritası

- Mevcut: anonim MQTT, HTTP web UI.
- Üretim için: MQTT TLS, web UI'da basic auth, OTA imzalı firmware (`Update.setMD5`).

## Pin Atamaları

| Çevre | ESP32 GPIO |
|---|---|
| RS485 RX (RO) | 16 |
| RS485 TX (DI) | 17 |
| RS485 DE/RE | 4 |
| I2C SDA (OLED) | 21 |
| I2C SCL (OLED) | 22 |
