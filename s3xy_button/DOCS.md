# S3XY virtuel knap

Denne add-on udgiver sig for at være Enhance Autos Bluetooth-knap (`ENH_BTN`), så du kan styre
din S3XY Commander fra Home Assistant. Du får en enhed med tre knapper — **Tryk**, **Dobbelttryk**
og **Langt tryk** — og en sensor, **Commander forbundet**.

## Krav
- **Mosquitto broker**-add-on'en og **MQTT**-integrationen. Add-on'en henter selv MQTT-login.
- Bilen skal holde inden for ca. **10 m** af Home Assistant-maskinen.
- Gerne en **ekstra USB-Bluetooth-dongle**, så HA's egen Bluetooth-integration ikke deler adapter med knappen.

## Indstillinger
| Indstilling | Forklaring |
|---|---|
| `bt_adapter` | Fx `hci1` for en USB-dongle. Tom = standardadapteren. |
| `btn_name` | Bluetooth-navnet. Lad det være `ENH_BTN`. |

Bruger du en ekstra dongle, så gå til *Indstillinger → Enheder & tjenester → Bluetooth* og
**ignorér/deaktivér** donglen dér, så HA ikke også bruger den.

## Parring
1. Start add-on'en og åbn fanen **Log**.
2. På S3XY Commander: **Buttons → Add S3XY Button**.
3. I loggen skal du se `Pairing confirmed`, `ID write: b6` og `Commander initialized`.
4. Sensoren **Commander forbundet** skifter til *Forbundet*.

## Brug
```yaml
action: button.press
target:
  entity_id: button.s3xy_virtuel_knap_tryk
```

## Fejlsøgning
- **`Could not locate bluetooth adapter`**: Tjek `bt_adapter`, eller lad feltet være tomt.
- **Ingen parring**: Prøv en dedikeret dongle, og flyt den tættere på bilen (USB-forlænger).
- **Tryk virker ikke**: Commanderen skal være forbundet (se sensoren).
