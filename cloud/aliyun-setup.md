# Aliyun IoT Setup Notes

This file documents setup steps only. Do not place real credentials or secrets in the repository.

## Manual Setup Checklist For Three Devices
1. Create one Aliyun IoT product for the ESP32 zone node firmware.
2. Register three devices under that product:
   - one device for `ZONE_A`
   - one device for `ZONE_B`
   - one device for `ZONE_C`
3. Bring up `ZONE_A` first, validate one device end to end, and only then copy the same process to `ZONE_B` and `ZONE_C`.
4. On the deployment machine, copy `firmware/zone_node/src/config_local.example.h` to `firmware/zone_node/src/config_local.h`.
5. Record the following shared values in `config_local.h` locally:
   - Wi-Fi SSID
   - Wi-Fi password
   - MQTT broker endpoint
   - ProductKey
   - MQTT port if different from the default placeholder
6. Record the following per-device values in the three zone blocks of `config_local.h`:
   - Zone ID
   - DeviceName
   - DeviceSecret
   - MQTT client ID
   - MQTT username
   - MQTT password or signature
7. Confirm the placeholder topic pattern for each device:
   - uplink: `/<productKey>/<deviceName>/user/fire/up`
   - downlink: `/<productKey>/<deviceName>/user/fire/down`
8. Build the matching firmware image for each device:
   - Zone A: `pio run -e esp32-zone-a`
   - Zone B: `pio run -e esp32-zone-b`
   - Zone C: `pio run -e esp32-zone-c`
9. Upload each image to the correct ESP32 node and label the boards physically.
10. Use the provided `cloud/example-payloads.json` file to test JSON payload shape before building any cloud-side rules.
11. Verify that every node still behaves correctly locally when Wi-Fi or MQTT is unavailable.

## Downlink Commands To Test
- `relay on`
- `relay off`
- `buzzer mute`
- `buzzer unmute`
- `request status publish`

## Safety Notes
- Remote relay commands are temporary manual overrides for demonstration only.
- Remote buzzer mute is temporary and must not change the actual local state machine state.
- Cloud connectivity must never be required for local warning evaluation or actuator control.
- Real credentials must remain only in the local untracked `config_local.h` file.
