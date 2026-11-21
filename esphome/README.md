# S3XY virtual button on an ESP32 (ESPHome)

An alternative to the Home Assistant add-on: an ESP32 placed close to the car pretends to be the
Enhance Auto `ENH_BTN` button. It talks to Home Assistant through the native ESPHome API, so
neither MQTT nor the add-on is needed.

Use this when the Home Assistant machine is too far from the car. The Commander sits inside the
car, and a real button is only centimetres away from it, so the signal must be strong.

## What you need
- An ESP32 with Bluetooth: original ESP32, ESP32-S3 or ESP32-C3. **Not** ESP32-S2.
- A USB power supply near the car (carport, garage, a window facing the driveway).
- The **ESPHome Device Builder** add-on in Home Assistant, or the `esphome` command line tool (2026.11 or newer).

## Install
1. In ESPHome Device Builder, create a new device and replace its YAML with `s3xy-button.yaml`.
2. Set `board` under `substitutions` to match your board, e.g. `esp32dev`, `esp32-s3-devkitc-1` or `esp32-c3-devkitm-1`.
3. Make sure `wifi_ssid`, `wifi_password` and `api_encryption_key` exist in your ESPHome secrets.
4. Install it. The first time over USB, after that over Wi-Fi.
5. Home Assistant discovers the device. Add it under *Settings → Devices & services*.

## M5StickC Plus
Use `m5stickc-plus.yaml` instead. Copy **both** YAML files to your ESPHome config folder, since it
includes `s3xy-button.yaml`. Extras:
- The red LED lights up while the Commander is connected.
- The big front button (M5) sends single, double and long presses. A single press waits 300 ms after release for a second tap. Holding the button for 600 ms sends a long press while the button is still down.
- The screen turns on at power-up and shows whether the Commander and Home Assistant are connected.
  The display's power and backlight come from the AXP192 power chip, which is switched on over I2C at boot.

ESPHome warns about GPIO10 (flash) and GPIO5/GPIO15 (strapping pins). On the M5StickC Plus these are
the LED and the display, so the warnings can be ignored. Keep it on USB power; the 120 mAh battery only lasts a few hours.

## Pairing
1. Open the device's logs in ESPHome Device Builder.
2. Sit in the car (awake) and open **Buttons → Add S3XY Button** in the S3XY app.
3. The log should show `Commander connected`, then `ID write: B6` and `Commander initialized`.

## Entities
- **Press**, **Double press**, **Long press** (buttons)
- **Commander connected** (binary sensor)

## If you also run the add-on
Stop the add-on (or remove the button from the Commander first). Both advertise as `ENH_BTN`.
