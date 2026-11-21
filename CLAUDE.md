# CLAUDE.md — s3xy-ha-bridge

Handover note for Claude Code. The owner (Erik) writes in Danish; reply in Danish unless asked otherwise.

## Goal
Control an Enhance Auto **S3XY Commander (Gen 2)** in a Tesla from **Home Assistant** without an ESP32.
A Linux machine with Bluetooth (BlueZ) pretends to be the official BLE button `ENH_BTN`, and HA presses it via MQTT.

**Long-term goal:** talk to the Commander in both directions (read status/state, not just send button presses).

## Origin
Python port of [Beat-YT/s3xy-virtual-button](https://github.com/Beat-YT/s3xy-virtual-button) (ESP32/Arduino, MIT).
All protocol details below come from that repo's `src/S3XYButton.cpp`, which was reverse-engineered by sniffing real ENH_BTN <-> Commander traffic.

## Status (2026-11-21)
- `s3xy_ha_bridge.py` is written, compiles and imports, but has **not been tested on hardware yet**. No BLE was available where it was written.
- Erik runs **Home Assistant OS** and already has Mosquitto. Decision: run the bridge on the HA host **as an add-on** (`s3xy_button/`), using MQTT.
- The add-on builds and runs on Erik's HA OS (hci0, Intel adapter, shared with HA's passive Bluetooth scanning). MQTT works (`core-mosquitto`), and a Shelly proxy sees hci0 advertising.
- **First pairing attempt (v0.2.0) failed:** the Commander app said "Something went wrong" and the add-on logged nothing at all after start, i.e. no connection/pairing/write was ever seen. v0.3.0 makes the advertisement discoverable (flags like the ESP32's `0x06`) and logs BlueZ Connected/Paired/Bonded/ServicesResolved events, so the next attempt shows how far it gets.
- **Second attempt (v0.3.0) failed the same way** (no connection logged). nRF Connect (iPhone) at the car showed: `ENH_BTN` advertising correctly (name + service `00003d46-…`) but only **-92 dBm**, while the Tesla (`S…C`) and the Commander were at -55 dBm. **Main suspect now: range/signal strength.** A real button sits inside the car, centimetres from the Commander. **Verified with nRF Connect (v0.3.0):** connect, `ServicesResolved`, Just Works pairing (`RequestAuthorization` on the agent, iOS shows a pairing dialog), `Bonded=True, Paired=True`, service/characteristics as in the sketch, and reading `3d49` returns `SABRINASNS`. So the bridge works when it is reachable. Next: improve signal (USB dongle on an extension cable towards the car, or the ESPHome/ESP32 route).
- HA side: MQTT discovery creates `button` entities (Press / Double press / Long press) plus a `binary_sensor` "Commander connected". This needs Mosquitto + the MQTT integration in HA.

## Files
| File | Purpose |
|---|---|
| `repository.yaml` | Makes the GitHub repo installable as an HA add-on repository |
| `s3xy_button/s3xy_ha_bridge.py` | The whole bridge: BLE GATT server (bless/BlueZ) + pairing agent + MQTT |
| `s3xy_button/config.yaml` | Add-on manifest: `host_dbus: true`, `services: mqtt:need`, options `bt_adapter`, `btn_name` |
| `s3xy_button/build.yaml`, `Dockerfile` | Built on `ghcr.io/home-assistant/{arch}-base-python:3.13-alpine3.22` |
| `s3xy_button/run.sh` | bashio script: reads MQTT credentials from the Supervisor and options, then starts the bridge |
| `s3xy_button/CHANGELOG.md` | Release notes, shown by HA when an update is offered |
| `s3xy_button/DOCS.md`, `translations/` | Add-on documentation and option labels |
| `s3xy_button/requirements.txt` | bless (<0.4; 0.3.0 still uses dbus-next), paho-mqtt (v2 API), dbus-next |
| `esphome/s3xy-button.yaml` | ESPHome alternative for an ESP32 near the car: pure YAML on ESPHome's `esp32_ble_server` (2026.11+), native HA API, no MQTT |
| `esphome/README.md` | Install/pairing guide for the ESPHome variant |
| `s3xy-button.service` | systemd unit for non-HA-OS installs; MQTT settings are passed in as env vars |
| `README.md` | User guide (add-on install, manual install, pairing, HA usage) |

## BLE protocol (ENH_BTN)
- Advertised name `ENH_BTN`. The service UUID is in the scan response.
- Service `00003d46-87d2-479e-7e45-8551415a6de1`
  - Notify char `00003d50-…` (read, notify). Button events are sent here.
  - "ID" char `00003d49-…` (read/write, **encryption required**). Value is a 10-byte ID (`SABRINASNS` in the original). The Commander writes commands to it.
- Security: LE Secure Connections + bonding, "Just Works" (IO cap NoInputNoOutput), 16-byte key.
- Advertising in the sketch: flags `0x06`, appearance `0x0000` and the name in the advertisement; the complete 128-bit service UUID in the scan response. bless sets name + service UUID but no Discoverable, so the bridge swaps in `DiscoverableAdvertisement` (monkeypatches `bless.backends.bluezdbus.dbus.application.BlueZLEAdvertisement`; relies on bless 0.3.x internals).
- The ESP32 BLE library stores written values, so after `B6` a read of the ID char returns `b6`. The bridge does the same on purpose to match the proven implementation.
- Commander -> button (writes to ID char):
  - `B6` -> reply notify `C7 00 01` (init/version check)
  - `A1` -> unpair/disconnect request. Calls `org.bluez.Device1.Disconnect` on the connected device (like `g_server->disconnect(0)` in the sketch).
  - `A4 XX YY ZZ` -> reply notify `A4 00 XX YY` (rename, likely the S/E/X/Y tag)
- Button -> Commander (notify):
  - Single: `01`, `00`, `C1 01`, `00` (about 5–20 ms apart)
  - Double: `C1 02`
  - Long: `C3 01`
  - On connect, the ESP32 version "pokes" `00`, but `notifyBytes()` drops it unless notifications are already subscribed, which they never are right at connect. So the poke is effectively a no-op and is not ported.

## Implementation notes / known risks
- `bless` sends a notify by changing the `Value` property on D-Bus. Check that two identical packets in a row (`00` … `00` in single) are actually both sent. If not, the single sequence may fail.
- The `JustWorksAgent` is registered as BlueZ default agent. It can conflict with other agents on the system.
- HA's own Bluetooth integration uses the same BlueZ. Preferably use a separate adapter (`BT_ADAPTER=hci1`).
- bless's `is_connected()` means "a central is subscribed to notifications", not "a device is connected" (nRF Connect connected + paired without subscribing, and the sensor stayed off). That is fine for presses, which only work when subscribed. It is polled every 5 s for the binary_sensor. Connection events themselves are logged immediately via D-Bus `PropertiesChanged` on `org.bluez.Device1` (filtered to our adapter).
- The method signatures in `JustWorksAgent` use dbus-next string annotations (`"o"`, `"u"`, `"s"`). pyflakes reports them as false positives.

## How to test (first time on hardware)
1. Open the add-on's **Log** tab (or `journalctl -u s3xy-button -f` for a manual install), then on the Commander: Buttons -> Add S3XY Button.
2. Expected: `Pairing confirmed`, then `ID write: b6` and `Commander initialized`.
3. Press "Press" in HA and check that the Commander reacts.
4. Debug BLE: `btmon` on the host, or nRF Connect on a phone.

## Roadmap ideas
1. Get pairing + single/double/long working on real hardware.
2. ~~Handle `A1` (disconnect)~~ Done. Connect poke not needed (see protocol notes).
3. More virtual buttons (one per S/E/X/Y tag?), or configurable IDs.
4. **Two-way communication with the Commander:** the Commander itself advertises as `ENH_<6 hex>` (Erik's: `ENH_550422`) with service `5857a678-87c6-11eb-8dcd-0242ac130003`, presumably what the S3XY app talks to. Start there. Then sniff what else the Commander/app sends (nRF Connect + btmon/Wireshark), map characteristics and status, and expose them as HA sensors.
5. **ESPHome alternative (in progress, `esphome/`):** Erik chose this route because of the weak signal. It turned out no external component is needed: ESPHome 2026.11's `esp32_ble` has `name`, `io_capability: none`, `auth_req_mode: sc_bond`, `max_key_size: 16`, and `esp32_ble_server` has `read_encrypt`/`write_encrypt`, `on_write` (args `x` bytes, `id` conn id), `on_connect`/`on_disconnect` and `set_value`/`notify` actions. `A1` disconnect uses `esp_ble_gatts_close(id(ble_srv)->get_gatts_if(), id)`. Not done vs. the sketch: the ESP32 does not itself request encryption on connect (the BlueZ version did not either, and iOS paired fine). Original idea, kept for reference: an ESP32 near the car running ESPHome, talking natively to HA (no MQTT/add-on). Preferred route: an ESPHome external component wrapping the protocol from Beat-YT's `S3XYButton.cpp` (proven against a real Commander). ESPHome's built-in `esp32_ble_server` is an option only if it supports the encryption-required characteristic + LE Secure Connections bonding the Commander needs (unverified). Note: HA's Bluetooth proxies (Shelly, ESPHome proxy) cannot be used, since they only scan/connect as a central and cannot act as a peripheral.
6. ~~Package it as a Home Assistant add-on~~ Done (`s3xy_button/`). Bump `version` in `config.yaml` on every change so HA offers the update, and add an entry to `s3xy_button/CHANGELOG.md`.

## Conventions
- Keep it as one simple Python file until it actually needs splitting.
- Everything in the repo is in English: README, add-on docs, HA entity names, code and comments. (Chat with Erik in Danish.)
- Never commit MQTT passwords. Use env vars or a local override of the service file.
