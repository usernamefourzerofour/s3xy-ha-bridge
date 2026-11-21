# S3XY virtual button -> Home Assistant (no ESP32)

Python port of [Beat-YT/s3xy-virtual-button](https://github.com/Beat-YT/s3xy-virtual-button).
A Linux machine with Bluetooth (e.g. the Home Assistant host or a Raspberry Pi) pretends to be an
`ENH_BTN` button, and Home Assistant gets three buttons via MQTT: **Press**, **Double press** and
**Long press**, plus a sensor showing whether the Commander is connected.

## Requirements
- Linux with BlueZ and a Bluetooth adapter **within BLE range of the car** (typically ~10 m)
- An MQTT broker in Home Assistant (e.g. the Mosquitto add-on) + the MQTT integration
- Preferably an adapter that Home Assistant's Bluetooth integration is *not* using at the same time

## Install as a Home Assistant add-on (Home Assistant OS)
1. *Settings → Add-ons → Add-on store → ⋮ → Repositories*, and add
   `https://github.com/usernamefourzerofour/s3xy-ha-bridge`
2. Install **S3XY virtual button** and start it. The MQTT login is fetched from Mosquitto automatically.
3. See the add-on's *Documentation* tab for options and pairing.

## Manual install (Raspberry Pi OS / Debian, without HA OS)
```bash
sudo mkdir -p /opt/s3xy && sudo cp s3xy_button/s3xy_ha_bridge.py s3xy_button/requirements.txt /opt/s3xy/
sudo python3 -m venv /opt/s3xy/venv
sudo /opt/s3xy/venv/bin/pip install -r /opt/s3xy/requirements.txt
# Set MQTT_HOST/USER/PASS in the service file first (or use `systemctl edit`)
sudo cp s3xy-button.service /etc/systemd/system/
sudo systemctl daemon-reload && sudo systemctl enable --now s3xy-button
journalctl -u s3xy-button -f
```

## Pairing
1. Start the add-on/service. It advertises as `ENH_BTN`.
2. On the S3XY Commander: **Buttons -> Add S3XY Button**.
3. The log should show `Commander initialized`, and in HA "Commander connected" changes to *Connected*.

## Usage in HA
The buttons are called e.g. `button.s3xy_virtual_button_press`, so they can be used in automations, scripts and dashboards:
```yaml
action: button.press
target:
  entity_id: button.s3xy_virtual_button_press
```

## Troubleshooting
- No pairing: run `bluetoothctl show` and check `Powered: yes`. Try `BT_ADAPTER=hci1` for a USB dongle.
- Presses do nothing: the Commander must be connected (see the sensor), and the car must be nearby.

## License
Based on [Beat-YT/s3xy-virtual-button](https://github.com/Beat-YT/s3xy-virtual-button) (MIT). See `LICENSE`.
