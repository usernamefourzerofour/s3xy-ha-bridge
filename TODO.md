# Open work

## 1. Verify the double-press BLE sequence

Approved and implemented in firmware 0.1.5. Build and hardware result pending. Keep pairing and the physical button path as the baseline. With the car parked and the screen showing `Commander ready`, assign a known-working action such as cabin lights solely to double press. Trigger the HA button once per trial.

Compare the failed 0.1.4 standalone `C1 02` baseline with 0.1.5's `01 → 00 → 01 → 00 → C1 02` sequence. The four gaps are 100, 150, 100 and 10 ms. Save the five `Double press TX` log lines and record the app and vehicle result. A local successful return shows acceptance for transmission. The action in the vehicle is the acceptance check. If the candidate fails, compare with a known-good physical S3XY Button or equivalent verified reference trace before settling on a packet sequence.

## 2. Verify long press and connect the physical button

Run a separate controlled HA long-press test with only the long-press action assigned. Once the BLE sequence for each gesture works, connect the physical button to the validated implementation. Retest single, double and long presses and confirm one action per gesture.

## 3. Revisit hold-to-pair separately

Deferred. Keep automatic advertising as the baseline until gesture tests are complete. Reintroduce a pairing gesture only as a separate change with hardware checks for initial pairing, reconnect and remove/re-add.
