#!/usr/bin/with-contenv bashio
# Pass MQTT credentials from the Mosquitto add-on and the add-on options to the bridge.

export MQTT_HOST="$(bashio::services mqtt 'host')"
export MQTT_PORT="$(bashio::services mqtt 'port')"
export MQTT_USER="$(bashio::services mqtt 'username')"
export MQTT_PASS="$(bashio::services mqtt 'password')"

export BTN_NAME="$(bashio::config 'btn_name')"
if bashio::config.has_value 'bt_adapter'; then
    export BT_ADAPTER="$(bashio::config 'bt_adapter')"
fi

bashio::log.info "Starter S3XY-bridge (MQTT: ${MQTT_HOST}:${MQTT_PORT}, adapter: ${BT_ADAPTER:-standard})"
exec python3 -u /app/s3xy_ha_bridge.py
