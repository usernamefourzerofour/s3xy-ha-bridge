# Decisions and lessons

## 2026-11-22. Restore automatic pairing

[0.1.4](https://github.com/usernamefourzerofour/s3xy-ha-bridge/commit/2dc5f0dabc75676fe81bf1e4e35cf71e67fc64b1) disables hold-to-pair in the M5StickC Plus package. A prior hold-to-pair build failed to pair after the owner removed the button in the app. In 0.1.4 the M5Stick advertises automatically and can remove the stored bond when it receives `A1`. App removal alone does not establish that `A1` reached the device. A later restart restored pairing. If the authentication timeout returns, collect the first attempt after restarting both devices before changing firmware or clearing bonds. An [Espressif report](https://github.com/espressif/esp-idf/issues/19286) describes a similar log pattern. [ESPHome 2026.11.0](https://esphome.io/changelog/2026.11.0/) already contains a workaround for that reported issue, so the pattern alone does not identify the cause here.

For a M5StickC Plus power cycle, unplug USB, hold the side power button for about 6 s to turn off, then about 2 s to turn on. A brief press did not restart the owner's device. [M5Stack hardware README](https://github.com/m5stack/M5StickC-Plus#readme)

## 2026-11-22. Test the BLE gesture before retuning the switch

[Local gesture classification](https://github.com/usernamefourzerofour/s3xy-ha-bridge/commit/4aa8f75a86939e31a6c983d5f4cc9242f91e1598) was simulated but did not establish Commander compatibility. A [later change](https://github.com/usernamefourzerofour/s3xy-ha-bridge/commit/d0c84252ac5d5820ce7a064213a4d50a826047fb) moved the physical button to raw down/up events. The failed HA double-press test bypasses both approaches. Verify the BLE sequence before changing physical click timing again.

## 2026-11-22. Verify the build and both Git references

The ESPHome package and its external BLE component each have a Git reference. A package-only rollback can still use the current BLE code. Pin both for a controlled rollback. When building from `main`, account for the package cache. The full package syntax supports `refresh: always`. Check the S3XY version on the display after flashing. [ESPHome package documentation](https://esphome.io/components/packages/#remotegit-packages)

The HA repair for `ESPHome Web acf300` referred to the old registration. The new `s3xy-button` uses the current API key from its own YAML. Device Builder's Online badge confirms reachability. The S3XY pairing path uses BLE independently of HA. [Home Assistant integration documentation](https://www.home-assistant.io/integrations/esphome/)
