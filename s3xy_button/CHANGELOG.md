# Changelog

## 0.3.0
- Advertise as discoverable (LE General Discoverable flag), like the real button. The first pairing attempt failed without the Commander ever connecting, and this is the most likely cause.
- Log Bluetooth events: connected, paired, bonded, services resolved and ID reads. The log now shows how far a pairing attempt gets.
- Disconnect when the Commander sends an unpair request (`A1`).
- The ID characteristic stores written values again, matching the original ESP32 implementation.

## 0.2.0
- All text is now in English: device name **S3XY virtual button**, buttons **Press**, **Double press** and **Long press**, sensor **Commander connected**, add-on docs and log messages.
- Entity IDs for new installs change, e.g. `button.s3xy_virtual_button_press`. Existing installs keep their old entity IDs.

## 0.1.0
- First release as a Home Assistant add-on.
- Emulates the `ENH_BTN` Bluetooth button and exposes it to Home Assistant via MQTT discovery.
- Fetches the MQTT login from the Mosquitto add-on automatically.
- Options: `bt_adapter` and `btn_name`.
