# S3XY virtuel knap -> Home Assistant (uden ESP32)

Python-port af [Beat-YT/s3xy-virtual-button](https://github.com/Beat-YT/s3xy-virtual-button).
En Linux-maskine med Bluetooth (fx en Raspberry Pi) udgiver sig for at være en `ENH_BTN`-knap,
og Home Assistant får tre knapper via MQTT: **Tryk**, **Dobbelttryk** og **Langt tryk**,
plus en sensor, der viser om Commanderen er forbundet.

## Krav
- Linux med BlueZ og en Bluetooth-adapter **inden for BLE-rækkevidde af bilen** (typisk ~10 m)
- En MQTT-broker i Home Assistant (fx Mosquitto-add-on'en) + MQTT-integrationen
- Brug helst en adapter, som Home Assistants Bluetooth-integration *ikke* bruger samtidig

## Installation som Home Assistant-add-on (Home Assistant OS)
1. *Indstillinger → Add-ons → Add-on butik → ⋮ → Repositories*, og tilføj
   `https://github.com/usernamefourzerofour/s3xy-ha-bridge`
2. Installér **S3XY virtuel knap** og start den. MQTT-login hentes automatisk fra Mosquitto.
3. Se fanen *Dokumentation* i add-on'en for indstillinger og parring.

## Installation (Raspberry Pi OS / Debian, uden HA OS)
```bash
sudo mkdir -p /opt/s3xy && sudo cp s3xy_button/s3xy_ha_bridge.py s3xy_button/requirements.txt /opt/s3xy/
sudo python3 -m venv /opt/s3xy/venv
sudo /opt/s3xy/venv/bin/pip install -r /opt/s3xy/requirements.txt
# Ret MQTT_HOST/USER/PASS i servicefilen først
sudo cp s3xy-button.service /etc/systemd/system/
sudo systemctl daemon-reload && sudo systemctl enable --now s3xy-button
journalctl -u s3xy-button -f
```

## Parring
1. Start servicen. Den reklamerer som `ENH_BTN`.
2. På S3XY Commander: **Buttons -> Add S3XY Button**.
3. I loggen skal du se `Commander initialized`, og i HA skifter "Commander forbundet" til *Forbundet*.

## Brug i HA
Knapperne hedder fx `button.s3xy_virtuel_knap_tryk`, så de kan bruges i automationer, scripts og dashboards:
```yaml
action: button.press
target:
  entity_id: button.s3xy_virtuel_knap_tryk
```

## Fejlsøgning
- Ingen parring: kør `bluetoothctl show` og tjek `Powered: yes`. Prøv evt. `BT_ADAPTER=hci1` til en USB-dongle.
- Tryk virker ikke: Commanderen skal være forbundet (se sensoren), og bilen skal være i nærheden.

## Licens
Baseret på [Beat-YT/s3xy-virtual-button](https://github.com/Beat-YT/s3xy-virtual-button) (MIT). Se `LICENSE`.
