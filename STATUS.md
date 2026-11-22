# Project status

Updated 2026-11-22. Active hardware is the M5StickC Plus with ESPHome 2026.11.0 and S3XY Commander. The ESPHome variant uses the native Home Assistant API. The Linux/BlueZ MQTT add-on remains an alternative, with its earlier test history in [CLAUDE.md](CLAUDE.md).

## Verified on hardware

- The M5Stick display showed S3XY firmware **0.1.4**, from [the automatic-pairing change](https://github.com/usernamefourzerofour/s3xy-ha-bridge/commit/2dc5f0dabc75676fe81bf1e4e35cf71e67fc64b1). The device uses [the M5StickC Plus package](esphome/m5stickc-plus.yaml).
- Pairing recovered after a restart. Earlier attempts connected at BLE level, logged an LTK negative reply, then timed out after about 30 s with authentication reason `0x63`. At 15:32:35 authentication succeeded, notifications were enabled, and the firmware answered Commander's `B6` initialization request. Reconnects at 15:34 and 15:43 also authenticated and enabled notifications. The previous key state on each device was not established.
- The owner reported that Home Assistant single press can execute the assigned action. A controlled test with the known-working action assigned only to double press did not execute it. The device logged `Sending double press` at 15:47:07.447 and 15:47:57.932 while Commander notifications were enabled. These log lines precede the notify call. They show script execution and an attempted send, with delivery and interpretation by Commander still unverified.
- The front button logged both physical press/release pairs at 15:47:50.028–15:47:50.436, with 199 ms from the first release to the second press. The current physical button path sends only `01` and `00`. A double-click action still did not execute. Long press has also been reported as failing, but this log does not establish the result of a controlled HA long-press test.

Evidence is the owner-provided ESPHome log `s3xy-button-logs (2).txt` from 2026-11-22, lines 167–176, 188–189, 224–229, 250 and 264–279, plus the owner's reported app and vehicle observations. The attachment stays outside the repo because it contains local network identifiers. Hardware observations are distinct from code review and earlier simulated timing checks.

## Current diagnosis

The failed HA double-press test bypasses the physical click detector. In [the ESPHome scripts](esphome/s3xy-button.yaml), HA single sends `01 → 00 → C1 01 → 00`, whereas HA double sends `C1 02` by itself and HA long sends `C3 01` by itself. These sequences come from the [reference implementation](https://github.com/Beat-YT/s3xy-virtual-button/blob/master/src/S3XYButton.cpp). Compatibility of the standalone double and long notifications with this Commander setup remains unverified. The app's press animation shows a press indication, with gesture classification and action execution still separate.

See [TODO.md](TODO.md) for the next test and [DECISIONS.md](DECISIONS.md) for prior changes and recovery notes.
