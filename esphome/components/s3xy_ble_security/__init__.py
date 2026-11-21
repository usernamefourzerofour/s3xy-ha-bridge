import esphome.codegen as cg
from esphome.components import esp32_ble, esp32_ble_server
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ["esp32_ble_server"]

S3XYBLESecurity = cg.esphome_ns.namespace("s3xy_ble_security").class_("S3XYBLESecurity", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(S3XYBLESecurity),
        cv.GenerateID("ble_id"): cv.use_id(esp32_ble.ESP32BLE),
        cv.GenerateID("server_id"): cv.use_id(esp32_ble_server.BLEServer),
        cv.Required("notify_id"): cv.use_id(esp32_ble_server.BLECharacteristic),
        cv.Optional("hold_to_pair", default=False): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    ble = await cg.get_variable(config["ble_id"])
    server = await cg.get_variable(config["server_id"])
    notify = await cg.get_variable(config["notify_id"])
    security = cg.new_Pvariable(config[CONF_ID], ble, server, notify)
    await cg.register_component(security, config)
    esp32_ble.register_gatts_event_handler(ble, security)
    if config["hold_to_pair"]:
        cg.add(security.enable_hold_to_pair())
