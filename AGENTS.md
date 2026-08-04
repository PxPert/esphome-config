# AGENTS.md — ESPHome Home Assistant Configuration

## Repo structure

- **Root level** (`*.yaml`): One YAML per physical device. Each file defines a single ESPHome node (e.g. `caldaia.yaml`, `tx_ultimate_06_soggiorno_cucina.yaml`, `muse-luxe.yaml`).
- **`includes/`**: Reusable fragments included via `!include` from device YAMLs (e.g. `mqtt_client.yaml`, `tx_ultimate_base.yaml`, `tx_ultimate_voice_assistant.yaml`).
- **`components/`**: Custom ESPHome external components sourced from `/config/components` on the Home Assistant filesystem (e.g. `tx_ultimate_touch`, `rc5x`, `atapi`).
- **`fonts/`**, **`audio/`**: Custom fonts and WAV files for displays/voice.
- **`secrets.yaml`**: All secrets (WiFi, MQTT, API, OTA) are loaded via `!secret` from this file. It is gitignored.
- **`packages/`**: MQTT discovery topics are published here by the `mqtt_client: !include` fragment.
- **`.esphome/`**: Generated build cache (ignore).
- **Remote**: `gogs@git.pxpert.cloud:pxpert/esphome.git` (branch `master`).

## Device YAML pattern

Every device file follows this shape:

```yaml
esphome:
  name: device_id
  friendly_name: "Human Name"
esp???: ...   # board, framework, etc.
...
packages:
  # optional fragments to include for shared concerns
  syslog: !include includes/syslog.yaml
  mqtt_client: !include includes/mqtt_client.yaml
```

When editing a device file, **do not** move or rename the `name:` substitution — it must match the filename stem (e.g. `tx_ultimate_06_soggiorno_cucina.yaml` → `name: tx_ultimate_06_soggiorno_cucina`).

## Key files to read first

1. `secrets.yaml` — credentials, MQTT broker, BLE MACs.
2. `includes/tx_ultimate_base.yaml` — TX Ultimate base pin map, globals, and framework config (ESP-IDF, PSRAM).
3. `includes/mqtt_client.yaml` — MQTT broker connection with discovery disabled.

## TX Ultimate specific

TX Ultimate devices use a layered include pattern:

- **`tx_ultimate_base.yaml`** defines: GPIO pin layout (relays, UART, touch panel), globals, ESP-IDF framework, PSRAM, custom component source (`/config/components`), and the `tx_ultimate_touch` component with callbacks for press/release/long-touch/swipe.
- **`tx_ultimate_base_scripts.yaml`**, **`tx_ultimate_base_leds.yaml`**, **`tx_ultimate_base_speaker.yaml`**: additional base functionality.
- **Device YAML** layers on top via `packages:` with: `device_base`, optional MQTT/VA/LEDs/light/switch fragments.
- **LED zone files** (`tx_ultimate_leds_1_zone.yaml` through `tx_ultimate_leds_4_zone.yaml`): pick exactly one based on the number of virtual buttons on the panel.
- Voice assistant requires MEMS mic GPIOs (`mic_lrclk_pin`, `mic_bclk_pin`, `mic_sdata_pin`) passed as `vars:`.

## GPIO pin map (TX Ultimate, from `tx_ultimate_base.yaml`)

| Pin   | Function     |
|-------|--------------|
| GPIO18| relay_1      |
| GPIO17| relay_2      |
| GPIO27| relay_3      |
| GPIO23| relay_4      |
| GPIO21| vibra_motor  |
| GPIO26| pa_power     |
| GPIO13| status_led   |
| GPIO19| uart_tx      |
| GPIO22| uart_rx      |
| GPIO5 | touch power  |

## Framework quirks

- **ESP32** devices: use ESP-IDF framework (`cpu_frequency: 80MHz`, PSRAM `80MHz`).
- **ESP8266** devices (`caldaia`, `lettorecd`): use `esp12e` board.
- **M5Stack Atom Echo**: use `m5stack-atom` board, ESP-IDF, 240MHz.
- `mdns: disabled: True` is set on most devices to prevent collisions.
- `preferences.flash_write_interval: 6h` is set on most devices to limit flash wear.

## Adding a new device

1. Create `device_id.yaml` in the repo root.
2. Use `!secret` for all credentials (don't hardcode passwords).
3. Reference `includes/` fragments for shared concerns (MQTT, syslog).
4. If it uses TX Ultimate hardware, follow the layered include pattern from `tx_ultimate_00_base.yaml`.
5. Add a custom component in `components/` if needed, then reference it in `external_components`.

## Testing changes

- Run `esphome config <file>.yaml` to validate before deploying.
- After modifying `includes/` files, rebuild affected device YAMLs.
- Custom components in `components/` must be sourced from `/config/components` on the HA filesystem (not from this repo).
