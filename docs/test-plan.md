# Test Plan

## Build Verification
- Confirm `firmware/zone_node/src/config_local.h` exists before the first real build.
- Confirm exactly one PlatformIO zone environment is selected: `esp32-zone-a`, `esp32-zone-b`, or `esp32-zone-c`.
- Confirm the selected build resolves the expected `ZONE_A`, `ZONE_B`, or `ZONE_C` identity in serial boot logs.
- Confirm the firmware still builds when cloud placeholders remain in `config_local.h`; runtime should skip Wi-Fi and MQTT rather than breaking local logic.
- Confirm the build output includes the expected libraries for DHT, PubSubClient, and ArduinoJson.
- Confirm a ZONE_A build succeeds before preparing B and C.

## Bring-Up Checks
- Confirm the ESP32 boots and prints startup logs.
- Confirm the selected zone identity is visible in serial logs.
- Confirm each module initializes without blocking the main loop.
- Confirm the node still runs locally if `config_local.h` contains placeholders or cloud connectivity is unavailable.

## Sensor Validation
- Verify MQ-2 readings change when smoke simulation is applied safely.
- Verify DHT11 values can be read and handled when unavailable.
- Verify flame sensor state transitions are logged clearly.

## Logic Validation
- Confirm deterministic transition behavior for normal, warning, alarm, and linkage states.
- Confirm thresholds and timing values can be adjusted in shared configuration.
- Confirm the system remains responsive without long blocking delays.

## Output Validation
- Verify green, yellow, and red LED behavior for each local state.
- Verify buzzer actuation matches warning or alarm expectations.
- Verify relay activates only in linkage state unless a temporary cloud demo override is active.

## Local Demo Procedure
1. Build and upload the firmware to the ESP32 zone node.
2. Open the serial monitor at `115200` baud.
3. Enter `help` and confirm the command list is printed.
4. Enter `status` and confirm the current state, sensor values, and override status are shown.
5. Enter `demo on` to enable controlled sensor overrides.
6. Enter `set mq2 350` and wait at least the warning persistence duration. Confirm a `NORMAL -> WARNING` transition is logged and the yellow LED turns on.
7. Enter `set flame 1` and confirm an immediate transition to `ALARM`, with the red LED and buzzer active.
8. Keep `set flame 1` active longer than the alarm persistence duration and confirm a transition to `LINKAGE`, with the relay turning on.
9. Enter `set flame 0` and `clear overrides`. Confirm the relay stays on for the configured linkage hold duration, then returns to the appropriate lower state.
10. Enter `demo off` and confirm the firmware returns to real sensor inputs only.
11. Enter `selftest` and confirm the startup actuator test sequence can be rerun from the serial monitor.

## Three-Zone Bring-Up Checklist
1. Copy `firmware/zone_node/src/config_local.example.h` to `firmware/zone_node/src/config_local.h` on the deployment machine.
2. Fill the shared local Wi-Fi and Aliyun product values once.
3. Fill the Zone A, Zone B, and Zone C credential blocks in the same local config file.
4. Build and upload Zone A with `pio run -e esp32-zone-a -t upload`.
5. Verify Zone A locally before preparing B and C.
6. Build and upload Zone B with `pio run -e esp32-zone-b -t upload`.
7. Build and upload Zone C with `pio run -e esp32-zone-c -t upload`.
8. Label each ESP32 physically so the flashed zone matches the installed hardware node.
9. Verify each node reports the correct `zone` value in serial logs and telemetry.
10. Verify each node can still enter WARNING, ALARM, and LINKAGE locally even if Wi-Fi is disabled.
11. Verify the downlink relay and buzzer demo commands affect only the targeted device.

## Cloud Validation
- Confirm each device connects with its own per-zone MQTT identity.
- Confirm telemetry from each zone appears with the correct `zone` field.
- Confirm downlink commands sent to one device do not affect the other two zones.
- Confirm the node continues local warning evaluation during Wi-Fi or MQTT outages.
