# Open work

## 1. Verify the double- and long-press BLE sequences

Implemented in firmware 0.1.6. Build and hardware result pending. Keep pairing and the physical button path as the baseline. With the car parked and the screen showing `Commander ready`, assign a known-working action such as cabin lights to one gesture at a time. Trigger the HA button once per trial.

- Double press: `01 → 00 → 01 → 00 → C1 02 → 00`. The five gaps are 100, 150, 100, 10 and 10 ms. Expect six `Double press TX` log lines.
- Long press: `01 → C3 01 → 00`. The gaps are 1000 and 10 ms. Expect three `Long press TX` log lines.

The 0.1.4 baseline sent `C1 02` and `C3 01` alone, as in the reference sketch, and the double-press action did not run. 0.1.6 copies the structure of the working single press (`01 → 00 → C1 01 → 00`), including the closing `00`. A local successful return shows acceptance for transmission. The action in the vehicle is the acceptance check. If a candidate fails, compare with a known-good physical S3XY Button or equivalent verified reference trace before settling on a packet sequence.

## 2. Connect the physical button

Once the BLE sequence for each gesture works, connect the physical button to the validated implementation. Retest single, double and long presses and confirm one action per gesture.

## 3. Revisit hold-to-pair separately

Deferred. Keep automatic advertising as the baseline until gesture tests are complete. Reintroduce a pairing gesture only as a separate change with hardware checks for initial pairing, reconnect and remove/re-add.
