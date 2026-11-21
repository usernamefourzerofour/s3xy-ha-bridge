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
- Not yet deployed. Open question: run it on the HA host itself or on a separate Pi close to the car. BLE range is about 10 m.
- HA side: MQTT discovery creates `button` entities (Tryk / Dobbelttryk / Langt tryk) plus a `binary_sensor` "Commander forbundet". This needs Mosquitto + the MQTT integration in HA.

## Files
| File | Purpose |
|---|---|
| `s3xy_ha_bridge.py` | The whole bridge: BLE GATT server (bless/BlueZ) + pairing agent + MQTT |
| `s3xy-button.service` | systemd unit; MQTT settings are passed in as env vars |
| `requirements.txt` | bless, paho-mqtt (v2 API), dbus-next |
| `README.md` | Danish user guide (install, pairing, HA usage) |

## BLE protocol (ENH_BTN)
- Advertised name `ENH_BTN`. The service UUID is in the scan response.
- Service `00003d46-87d2-479e-7e45-8551415a6de1`
  - Notify char `00003d50-…` (read, notify). Button events are sent here.
  - "ID" char `00003d49-…` (read/write, **encryption required**). Value is a 10-byte ID (`SABRINASNS` in the original). The Commander writes commands to it.
- Security: LE Secure Connections + bonding, "Just Works" (IO cap NoInputNoOutput), 16-byte key.
- Commander -> button (writes to ID char):
  - `B6` -> reply notify `C7 00 01` (init/version check)
  - `A1` -> unpair/disconnect request. **Currently only logged**, and no disconnect is done. TODO: call `org.bluez.Device1.Disconnect`.
  - `A4 XX YY ZZ` -> reply notify `A4 00 XX YY` (rename, likely the S/E/X/Y tag)
- Button -> Commander (notify):
  - Single: `01`, `00`, `C1 01`, `00` (about 5–20 ms apart)
  - Double: `C1 02`
  - Long: `C3 01`
  - On connect, the ESP32 version sends a "poke" `00`. **The Python version does not** (bless has no connect callback). Add it if pairing/init fails.

## Implementation notes / known risks
- `bless` sends a notify by changing the `Value` property on D-Bus. Check that two identical packets in a row (`00` … `00` in single) are actually both sent. If not, the single sequence may fail.
- The `JustWorksAgent` is registered as BlueZ default agent. It can conflict with other agents on the system.
- HA's own Bluetooth integration uses the same BlueZ. Preferably use a separate adapter (`BT_ADAPTER=hci1`).
- `is_connected()` is polled every 5 s for the binary_sensor.
- The method signatures in `JustWorksAgent` use dbus-next string annotations (`"o"`, `"u"`, `"s"`). pyflakes reports them as false positives.

## How to test (first time on hardware)
1. `journalctl -u s3xy-button -f`, then on the Commander: Buttons -> Add S3XY Button.
2. Expected: `Pairing confirmed`, then `ID write: b6` and `Commander initialized`.
3. Press "Tryk" in HA and check that the Commander reacts.
4. Debug BLE: `btmon` on the host, or nRF Connect on a phone.

## Roadmap ideas
1. Get pairing + single/double/long working on real hardware.
2. Handle `A1` (disconnect) and the connect poke properly.
3. More virtual buttons (one per S/E/X/Y tag?), or configurable IDs.
4. **Two-way communication with the Commander:** sniff what else the Commander/app sends (nRF Connect + btmon/Wireshark), map characteristics and status, and expose them as HA sensors.
5. Possibly package it as a Home Assistant add-on (needs `host_dbus: true` + Bluetooth access).

## Conventions
- Keep it as one simple Python file until it actually needs splitting.
- User-facing text (README, HA entity names) in Danish; code and comments in English.
- Never commit MQTT passwords. Use env vars or a local override of the service file.
