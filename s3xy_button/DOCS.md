# S3XY virtual button

This add-on pretends to be Enhance Auto's Bluetooth button (`ENH_BTN`), so you can control
your S3XY Commander from Home Assistant. You get a device with three buttons (**Press**,
**Double press** and **Long press**) and a sensor, **Commander connected**.

## Requirements
- The **Mosquitto broker** add-on and the **MQTT** integration. The add-on fetches the MQTT login itself.
- The car must be parked within about **10 m** of the Home Assistant machine.
- Preferably an **extra USB Bluetooth dongle**, so HA's own Bluetooth integration does not share an adapter with the button.

## Options
| Option | Description |
|---|---|
| `bt_adapter` | E.g. `hci1` for a USB dongle. Empty = default adapter. |
| `btn_name` | The Bluetooth name. Leave it as `ENH_BTN`. |

If you use an extra dongle, go to *Settings → Devices & services → Bluetooth* and
**ignore/disable** the dongle there, so HA does not use it as well.

## Pairing
1. Start the add-on and open the **Log** tab.
2. On the S3XY Commander: **Buttons → Add S3XY Button**.
3. The log should show `Pairing confirmed`, `ID write: b6` and `Commander initialized`.
4. The **Commander connected** sensor changes to *Connected*.

## Usage
```yaml
action: button.press
target:
  entity_id: button.s3xy_virtual_button_press
```

## Troubleshooting
- **`Could not locate bluetooth adapter`**: Check `bt_adapter`, or leave it empty.
- **No pairing**: Try a dedicated dongle and move it closer to the car (USB extension cable).
- **Presses do nothing**: The Commander must be connected (see the sensor).
