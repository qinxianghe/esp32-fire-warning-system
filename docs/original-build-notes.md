# Multi-Zone Library Fire Early Warning System

Undergraduate IoT graduation project for a 3-zone library fire early warning demo using ESP32 nodes in zones A, B, and C.

## Repository Structure
- `AGENTS.md`: repository-specific engineering rules for Codex and contributors
- `docs/`: architecture, pin mapping, MQTT topic planning, and test planning
- `firmware/zone_node/`: shared PlatformIO firmware project reused by all three zone nodes
- `firmware/zone_node/src/config.h`: shared firmware constants, thresholds, timing, and topic suffixes
- `firmware/zone_node/src/config_local.example.h`: tracked example for local per-zone Wi-Fi and Aliyun settings
- `firmware/zone_node/src/config_local.h`: local untracked file for real credentials and per-device zone values
- `cloud/`: Aliyun IoT setup notes and example payload shapes without real credentials

## System Scope
- Inputs per zone: MQ-2 smoke, DHT11 temperature/humidity, IR flame sensor
- Outputs per zone: green LED, yellow LED, red LED, buzzer
- Shared linkage output: one relay for linkage demo
- Cloud: Aliyun IoT over MQTT for telemetry upload and downlink command handling

## Configuration Structure
- Shared firmware behavior lives in `firmware/zone_node/src/config.h`.
- GPIO assignments are centralized in `firmware/zone_node/src/pins.h` and mirrored in `docs/pinmap.md`.
- Copy `firmware/zone_node/src/config_local.example.h` to `firmware/zone_node/src/config_local.h` before a real deployment.
- Put real Wi-Fi credentials, Aliyun broker settings, and per-device MQTT credentials only in `config_local.h`.
- `config_local.h` is ignored by git and must not be committed.
- Build exactly one zone profile at a time: `esp32-zone-a`, `esp32-zone-b`, or `esp32-zone-c`.

## Prepare Each Zone
1. Copy `firmware/zone_node/src/config_local.example.h` to `firmware/zone_node/src/config_local.h`.
2. Fill the shared local values once:
   - `APP_WIFI_SSID`
   - `APP_WIFI_PASSWORD`
   - `APP_ALIYUN_PRODUCT_KEY`
   - `APP_ALIYUN_BROKER`
   - `APP_ALIYUN_PORT`
3. Fill the per-device values for all three blocks in `config_local.h`:
   - `APP_ZONE_ID`
   - `APP_ALIYUN_DEVICE_NAME`
   - `APP_ALIYUN_DEVICE_SECRET`
   - `APP_ALIYUN_MQTT_CLIENT_ID`
   - `APP_ALIYUN_MQTT_USERNAME`
   - `APP_ALIYUN_MQTT_PASSWORD`
4. Build the matching PlatformIO environment:
   - Zone A: `pio run -e esp32-zone-a`
   - Zone B: `pio run -e esp32-zone-b`
   - Zone C: `pio run -e esp32-zone-c`
5. Upload the matching environment to the correct ESP32 board:
   - Zone A: `pio run -e esp32-zone-a -t upload`
   - Zone B: `pio run -e esp32-zone-b -t upload`
   - Zone C: `pio run -e esp32-zone-c -t upload`

## ZONE_A First Real Bring-Up
1. Copy `firmware/zone_node/src/config_local.example.h` to `firmware/zone_node/src/config_local.h`.
2. Fill the shared Wi-Fi and Aliyun product values in `config_local.h`.
3. Fill only the `APP_ZONE_PROFILE_A` device block first with the real `ZONE_A` device values.
4. From `firmware/zone_node/`, run `pio run -e esp32-zone-a`.
5. Upload `ZONE_A` with `pio run -e esp32-zone-a -t upload --upload-port <PORT>`.
6. Monitor `ZONE_A` with `pio device monitor --port <PORT> --baud 115200`.
7. Confirm the serial log shows `ZONE_A`, the startup self-test, stable sensor prints, and either a cloud placeholder warning or a real Wi-Fi or MQTT connection attempt.
8. Only after `ZONE_A` is stable, fill the `ZONE_B` and `ZONE_C` blocks and repeat with `esp32-zone-b` and `esp32-zone-c`.

## Local Demo Commands
- `help`: print available debug commands
- `status`: print current state, real sensor values, effective demo values, and override status
- `selftest`: rerun the local actuator self-test sequence
- `demo on` / `demo off`: enable or disable sensor overrides
- `set mq2 <value>`: override processed MQ-2 input for warning or alarm testing
- `set temp <value>`: override temperature input for warning or alarm testing
- `set flame <0|1>`: override stable flame input for alarm testing
- `clear overrides`: remove all demo overrides while keeping demo mode available

## Notes
- One shared firmware tree is reused for zones A, B, and C.
- Thresholds, timing values, and deterministic warning rules stay in tracked source.
- Real secrets, API keys, device triples, and final cloud endpoints stay only in local untracked config.
- Prefer non-blocking ESP32 loop design and clear `Serial` debug logs.

## Build
1. Install [PlatformIO](https://platformio.org/).
2. Open `firmware/zone_node/` as a PlatformIO project, or run `pio run` in that directory for the default `esp32-zone-a` environment.
3. Create `src/config_local.h` from `src/config_local.example.h` before the first real build.
4. Open the serial monitor at `115200` baud and type `help` to view local demo commands.

## Status
Shared local firmware, deterministic state logic, serial demo mode, and the Aliyun MQTT scaffold are implemented. Cloud-side rules, notification workflows, and final deployment credentials remain outside the repository.
