# Open work

## 1. Verify the double-press BLE sequence

Proposed experiment, pending the owner's go-ahead for firmware changes. Keep pairing and the physical button path as the baseline. With the car parked and the screen showing `Commander ready`, assign a known-working action such as cabin lights solely to double press. Trigger the HA button once per trial.

Compare the current standalone `C1 02` with a candidate sequence of two down/up pairs followed by `C1 02`. Record bytes, timing and the BLE stack's return status for each notification, plus the app and vehicle result. A local successful return shows acceptance for transmission. The action in the vehicle is the acceptance check. If the candidate fails, compare with a known-good physical S3XY Button or equivalent verified reference trace before settling on a packet sequence.

## 2. Verify long press and connect the physical button

Run a separate controlled HA long-press test with only the long-press action assigned. Once the BLE sequence for each gesture works, connect the physical button to the validated implementation. Retest single, double and long presses and confirm one action per gesture.

## 3. Revisit hold-to-pair separately

Deferred. Keep automatic advertising as the baseline until gesture tests are complete. Reintroduce a pairing gesture only as a separate change with hardware checks for initial pairing, reconnect and remove/re-add.
