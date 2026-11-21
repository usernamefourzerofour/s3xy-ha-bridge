#!/usr/bin/env python3
"""
S3XY virtual button -> Home Assistant bridge (no ESP32 needed).

Port of https://github.com/Beat-YT/s3xy-virtual-button to Python/BlueZ.
Any Linux machine with Bluetooth (Raspberry Pi, mini-PC, the HA host) within
BLE range of the car pretends to be an Enhance "ENH_BTN" button. Home Assistant
gets three buttons via MQTT discovery: single, double and long press.

Config via environment variables:
  MQTT_HOST (default: localhost)   MQTT_PORT (default: 1883)
  MQTT_USER / MQTT_PASS (optional)
  BT_ADAPTER (optional, e.g. hci1)  BTN_NAME (default: ENH_BTN)
"""

import asyncio
import json
import logging
import os

import paho.mqtt.client as mqtt
from bless import (
    BlessServer,
    GATTAttributePermissions,
    GATTCharacteristicProperties,
)
from dbus_next.service import ServiceInterface, method

log = logging.getLogger("s3xy")

# ---- Protocol (from the original ESP32 sketch) ----
SERVICE_UUID = "00003d46-87d2-479e-7e45-8551415a6de1"
CHAR_NOTIFY_UUID = "00003d50-87d2-479e-7e45-8551415a6de1"
CHAR_ID_UUID = "00003d49-87d2-479e-7e45-8551415a6de1"
BUTTON_ID = b"SABRINASNS"  # 10-byte ID exposed on the ID characteristic

PRESS_SEQUENCES = {
    "SINGLE": [b"\x01", b"\x00", b"\xc1\x01", b"\x00"],
    "DOUBLE": [b"\xc1\x02"],
    "LONG": [b"\xc3\x01"],
}

# ---- MQTT / Home Assistant ----
NODE = "s3xy_button"
CMD_TOPIC = f"{NODE}/press"
AVAIL_TOPIC = f"{NODE}/status"
CONN_TOPIC = f"{NODE}/connected"
DEVICE = {
    "identifiers": [NODE],
    "name": "S3XY virtual button",
    "manufacturer": "DIY",
    "model": "ENH_BTN emulator",
}


class JustWorksAgent(ServiceInterface):
    """BlueZ pairing agent that accepts 'Just Works' bonding like the real button."""

    def __init__(self):
        super().__init__("org.bluez.Agent1")

    @method()
    def Release(self):
        pass

    @method()
    def RequestConfirmation(self, device: "o", passkey: "u"):  # noqa: F821
        log.info("Pairing confirmed for %s", device)

    @method()
    def RequestAuthorization(self, device: "o"):  # noqa: F821
        log.info("Pairing authorized for %s", device)

    @method()
    def AuthorizeService(self, device: "o", uuid: "s"):  # noqa: F821
        pass

    @method()
    def Cancel(self):
        log.info("Pairing cancelled")


class S3XYButton:
    def __init__(self, loop: asyncio.AbstractEventLoop):
        self.loop = loop
        self.name = os.getenv("BTN_NAME", "ENH_BTN")
        kwargs = {"adapter": os.getenv("BT_ADAPTER")} if os.getenv("BT_ADAPTER") else {}
        self.server = BlessServer(name=self.name, loop=loop, **kwargs)
        self.server.read_request_func = self._on_read
        self.server.write_request_func = self._on_write
        self._lock = asyncio.Lock()

    async def start(self):
        s = self.server
        await s.add_new_service(SERVICE_UUID)
        await s.add_new_characteristic(
            SERVICE_UUID,
            CHAR_NOTIFY_UUID,
            GATTCharacteristicProperties.read | GATTCharacteristicProperties.notify,
            bytearray(b"\x00"),
            GATTAttributePermissions.readable,
        )
        await s.add_new_characteristic(
            SERVICE_UUID,
            CHAR_ID_UUID,
            GATTCharacteristicProperties.read | GATTCharacteristicProperties.write,
            bytearray(BUTTON_ID),
            GATTAttributePermissions.read_encryption_required
            | GATTAttributePermissions.write_encryption_required,
        )
        await s.start()
        await self._register_agent()
        log.info("Advertising as %s - add it on the Commander: Buttons -> Add S3XY Button", self.name)

    async def _register_agent(self):
        bus = self.server.bus
        path = "/s3xy/agent"
        bus.export(path, JustWorksAgent())
        intro = await bus.introspect("org.bluez", "/org/bluez")
        mgr = bus.get_proxy_object("org.bluez", "/org/bluez", intro).get_interface("org.bluez.AgentManager1")
        await mgr.call_register_agent(path, "NoInputNoOutput")
        await mgr.call_request_default_agent(path)
        # Allow bonding on the adapter
        adapter = self.server.adapter.get_interface("org.bluez.Adapter1")
        await adapter.set_pairable(True)

    # --- GATT handlers ---
    def _on_read(self, characteristic, **kwargs):
        return characteristic.value

    def _on_write(self, characteristic, value, **kwargs):
        if str(characteristic.uuid).lower() != CHAR_ID_UUID:
            characteristic.value = value
            return
        # Writes to the ID char are commands; keep the stored value as the
        # 10-byte button ID so later reads still return it.
        data = bytes(value)
        log.info("ID write: %s", data.hex(" "))
        if data == b"\xb6":  # init / version check
            self.loop.create_task(self._notify_seq([b"\xc7\x00\x01"]))
            log.info("Commander initialized")
        elif data == b"\xa1":  # unpair
            log.info("Commander requested unpair")
        elif len(data) == 4 and data[0] == 0xA4:  # rename
            self.loop.create_task(self._notify_seq([bytes([0xA4, 0x00, data[1], data[2]])]))
            log.info("Rename handled")

    # --- Notifications ---
    async def _notify_seq(self, packets):
        async with self._lock:
            char = self.server.get_characteristic(CHAR_NOTIFY_UUID)
            for p in packets:
                char.value = bytearray(p)
                self.server.update_value(SERVICE_UUID, CHAR_NOTIFY_UUID)
                await asyncio.sleep(0.02)

    async def press(self, kind: str):
        seq = PRESS_SEQUENCES.get(kind.upper())
        if not seq:
            log.warning("Unknown press type: %s", kind)
            return
        if not await self.server.is_connected():
            log.warning("Press %s ignored - Commander not connected", kind)
            return
        log.info("Press: %s", kind)
        await self._notify_seq(seq)


def setup_mqtt(loop, button: S3XYButton) -> mqtt.Client:
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=NODE)
    if os.getenv("MQTT_USER"):
        client.username_pw_set(os.getenv("MQTT_USER"), os.getenv("MQTT_PASS"))
    client.will_set(AVAIL_TOPIC, "offline", retain=True)

    def on_connect(c, userdata, flags, rc, props=None):
        log.info("MQTT connected (%s)", rc)
        for kind, label, icon in (
            ("SINGLE", "Press", "mdi:gesture-tap"),
            ("DOUBLE", "Double press", "mdi:gesture-double-tap"),
            ("LONG", "Long press", "mdi:gesture-tap-hold"),
        ):
            uid = f"{NODE}_{kind.lower()}"
            c.publish(
                f"homeassistant/button/{uid}/config",
                json.dumps({
                    "name": label, "unique_id": uid, "icon": icon,
                    "command_topic": CMD_TOPIC, "payload_press": kind,
                    "availability_topic": AVAIL_TOPIC, "device": DEVICE,
                }),
                retain=True,
            )
        c.publish(
            f"homeassistant/binary_sensor/{NODE}_connected/config",
            json.dumps({
                "name": "Commander connected", "unique_id": f"{NODE}_connected",
                "device_class": "connectivity", "state_topic": CONN_TOPIC,
                "availability_topic": AVAIL_TOPIC, "device": DEVICE,
            }),
            retain=True,
        )
        c.publish(AVAIL_TOPIC, "online", retain=True)
        c.subscribe(CMD_TOPIC)

    def on_message(c, userdata, msg):
        kind = msg.payload.decode(errors="ignore").strip()
        asyncio.run_coroutine_threadsafe(button.press(kind), loop)

    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(os.getenv("MQTT_HOST", "localhost"), int(os.getenv("MQTT_PORT", "1883")))
    client.loop_start()
    return client


async def main():
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    loop = asyncio.get_running_loop()
    button = S3XYButton(loop)
    await button.start()
    client = setup_mqtt(loop, button)

    last = None
    while True:  # report connection state to HA
        connected = await button.server.is_connected()
        if connected != last:
            client.publish(CONN_TOPIC, "ON" if connected else "OFF", retain=True)
            log.info("Commander %s", "connected" if connected else "disconnected")
            last = connected
        await asyncio.sleep(5)


if __name__ == "__main__":
    asyncio.run(main())
