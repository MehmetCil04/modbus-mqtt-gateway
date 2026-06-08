# ESP32 olmadan gateway'i simüle eder.
# Her 2 saniyede bir sahte voltage/current değeri MQTT'ye publish eder.
# Durdurmak için Ctrl+C.

$ErrorActionPreference = "Stop"
$tmp = Join-Path $env:TEMP "gw-sim-msg.json"

Write-Host "Gateway simulator. Ctrl+C ile durdurun." -ForegroundColor Cyan
Write-Host "Grafana: http://localhost:3000`n" -ForegroundColor Cyan

while ($true) {
    $ts = [int][double]::Parse((Get-Date -UFormat %s))
    $voltage = [math]::Round(230 + (Get-Random -Minimum -150 -Maximum 150) / 100, 2)
    $current = [math]::Round(2.3 + (Get-Random -Minimum -40 -Maximum 40) / 100, 3)

    # Voltage
    $json = "{`"value`":$voltage,`"unit`":`"V`",`"ts`":$ts,`"slave`":1,`"addr`":0}"
    Set-Content -Path $tmp -Value $json -Encoding ASCII -NoNewline
    docker cp $tmp gw-mosquitto:/tmp/msg.json | Out-Null
    docker exec gw-mosquitto mosquitto_pub -t "gateway/gw-01/voltage" -f /tmp/msg.json

    # Current
    $json = "{`"value`":$current,`"unit`":`"A`",`"ts`":$ts,`"slave`":1,`"addr`":1}"
    Set-Content -Path $tmp -Value $json -Encoding ASCII -NoNewline
    docker cp $tmp gw-mosquitto:/tmp/msg.json | Out-Null
    docker exec gw-mosquitto mosquitto_pub -t "gateway/gw-01/current" -f /tmp/msg.json

    Write-Host ("{0:HH:mm:ss}  V={1,7}V  I={2,6}A" -f (Get-Date), $voltage, $current)
    Start-Sleep -Seconds 2
}
