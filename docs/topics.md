# MQTT Topics and Payload Planning

This document is intentionally placeholder-only. Do not store real product keys, device names, secrets, or final endpoints here.

## Placeholder Topics
- Uplink telemetry topic: `/<productKey>/<deviceName>/user/fire/up`
- Downlink command topic: `/<productKey>/<deviceName>/user/fire/down`

## Telemetry Payload Fields
Telemetry JSON should include:
- `zone`
- `state`
- `mq2Raw`
- `mq2Processed`
- `temperatureC`
- `humidityPercent`
- `flameStableDetected`
- `buzzerActive`
- `relayActive`
- `demoModeActive`
- `activeReason`
- `uptimeMs`

## Downlink Command Payload Shape
Keep downlink JSON explicit and easy to test. Suggested commands:
- `{"command":"relay","value":"on"}`
- `{"command":"relay","value":"off"}`
- `{"command":"buzzer","value":"mute"}`
- `{"command":"buzzer","value":"unmute"}`
- `{"command":"request_status_publish"}`

## Notes
- Use one device first, then replicate the same topic pattern for zones B and C.
- Keep telemetry compact and thesis-friendly.
- Do not depend on cloud connectivity for local warning behavior.
