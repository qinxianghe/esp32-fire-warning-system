# AGENTS.md

## Project Goal
Build an undergraduate IoT graduation project for a multi-zone library fire early warning system using 3 ESP32 nodes for zones A, B, and C. Each zone reads MQ-2, DHT11, and IR flame sensors; drives green/yellow/red LEDs plus a buzzer; and participates in one shared relay linkage demo. The firmware must support Aliyun IoT MQTT telemetry upload and downlink command handling, while cloud rules and credentials remain documented externally and never hardcoded.

## Engineering Constraints
- Keep firmware modular: separate sensor reading, rule evaluation, output control, connectivity, configuration, and logging concerns.
- Use deterministic rule-based logic only. Do not add AI, ML, fuzzy inference, or opaque scoring unless the user explicitly requests and approves it.
- Design for scale from 1 zone to 3 zones with the same code structure, preferably via reusable zone state/config structs or classes.
- Make thresholds and timing values configurable, not embedded as magic numbers throughout the code.
- Never hardcode secrets, API keys, device triples, or final Aliyun endpoints. Use placeholders, config templates, or ignored local config files.
- Prefer non-blocking ESP32 loop design. Avoid long `delay()` usage except for brief hardware-safe cases with clear justification.
- Keep serial logging clear and actionable. Logs should identify zone, sensor, state transition, connectivity state, and command handling results.

## Coding Style
- Favor small, testable modules over monolithic `.ino` logic.
- Use clear names that reflect hardware purpose, for example `ZoneConfig`, `ZoneState`, `evaluateAlarmState()`, `publishTelemetry()`.
- Centralize pin maps, thresholds, debounce windows, and publish intervals in config definitions.
- Use `const`, enums, and structs where they improve readability and prevent accidental state drift.
- Keep state transitions explicit and easy to trace in logs.
- Add brief comments only where hardware behavior, timing, or safety logic is not obvious from the code itself.

## Done When
A firmware task is done when:
- The change preserves modular structure and does not introduce unnecessary refactors.
- The logic remains deterministic and configurable for all zones.
- Zone behavior is clear for normal, warning, and alarm states.
- Serial logs are sufficient to trace boot, sensor reads, rule decisions, MQTT connect or reconnect flow, publish events, and downlink handling.
- Secrets remain externalized and placeholders are used wherever real cloud values are required.
- The code can be built for the intended ESP32 target, or any build blocker is documented clearly.
- Basic validation steps are documented, including how to exercise sensor inputs, zone alarms, relay linkage behavior, and cloud message flow.

## What Codex Must Not Do
- Do not invent fake credentials, device triples, or production cloud endpoints.
- Do not perform destructive refactors without approval.
- Do not delete unrelated files, code, or documentation.
- Do not replace deterministic alarm rules with unnecessary abstraction or AI-style logic.
- Do not hide missing hardware assumptions; state them plainly.
