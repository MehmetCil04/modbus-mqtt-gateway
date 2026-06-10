# Build & Integrate — TODO

Bu chip kaynağı yazıldı ama henüz `chip.wasm` derlenmedi. İnternet hızlandığında
aşağıdaki 5 adımı uygulayın — yaklaşık **5-10 dakika** sürer.

## 1. WASI SDK indir (~150 MB)

PowerShell'de:

```powershell
$url = "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-33/wasi-sdk-33.0-x86_64-windows.tar.gz"
Invoke-WebRequest -Uri $url -OutFile "$env:USERPROFILE\wasi-sdk-33.tar.gz" -UseBasicParsing
```

Yavaşsa alternatif: tarayıcıdan aynı URL'ye gidin, indirme manager'ı kullanın.

## 2. Çıkart

```powershell
tar -xzf "$env:USERPROFILE\wasi-sdk-33.tar.gz" -C "$env:USERPROFILE"
& "$env:USERPROFILE\wasi-sdk-33.0-x86_64-windows\bin\clang.exe" --version
```

`clang version 19.x.x` görmelisiniz.

## 3. chip.wasm'i derle

```powershell
cd "C:\Users\mehme\Desktop\Yeni klasör (2)\modbus-mqtt-gateway\chips\wokwi-pzem-004t"
$clang = "$env:USERPROFILE\wasi-sdk-33.0-x86_64-windows\bin\clang.exe"
& $clang --target=wasm32 -Wall -Wno-unused-parameter -mcpu=mvp -nostdlib `
  "-Wl,--no-entry,--export-table,--initial-memory=65536,--max-memory=65536,--stack-first,-zstack-size=8192" `
  -o chip.wasm src/main.c
Get-Item chip.wasm | Select-Object Name, Length
```

~10 KB civarında bir `chip.wasm` dosyası oluşmalı.

## 4. firmware/diagram.json'u güncelle

`firmware/diagram.json` içine **OLED bağlantılarından sonra** şu parçayı ekle:

```json
{
  "type": "chip-wokwi-pzem-004t",
  "id": "meter",
  "top": 0,
  "left": 280,
  "attrs": {}
}
```

ve bağlantılara:
```json
[ "esp:17",    "meter:RX",  "#0099ff", [ "v100", "h150" ] ],
[ "esp:16",    "meter:TX",  "#aa55ff", [ "v120", "h150" ] ],
[ "esp:GND.2", "meter:GND", "black",   [ "v140", "h150" ] ],
[ "esp:5V",    "meter:VCC", "red",     [ "v-10", "h150" ] ]
```

ve `dependencies` bölümüne:
```json
"dependencies": {
  "chip-wokwi-pzem-004t": "../chips/wokwi-pzem-004t/chip.json"
}
```

ESP32 üzerindeki UART loopback wire'larını (`esp:17 → esp:5` ve `esp:16 → esp:18`)
silebilirsiniz — chip'le çakışırlar.

## 5. Wokwi'de test et

VS Code → Wokwi: Start Simulator. OLED'de `OK: 5+, ERR: 0` görmelisiniz.

Hâlâ çalışıyorsa: README'deki Wokwi bölümünü chip'li versiyona güncelle, commit + push.

## Sorun çıkarsa

- `make` komutu Windows'ta yoksa: yukarıdaki manuel clang komutunu kullanın.
- chip.wasm oluştu ama Wokwi tanımıyor: VS Code'u tamamen yeniden başlatın.
- ERR artıyor: chip 9600 baud'a ayarlı, master config'i de 9600 olduğundan emin olun
  (varsayılan zaten 9600, ama LittleFS'te eski config varsa override eder).
