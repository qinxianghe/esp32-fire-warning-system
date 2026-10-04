# Pin Map

Current placeholder GPIO assignments for the ESP32 Dev Module zone node. This document must match `firmware/zone_node/src/pins.h` exactly.

## Per-Zone GPIO Assignments
- MQ-2 analog input: GPIO34
- DHT11 data: GPIO4
- Flame sensor digital input: GPIO27
- Green LED output: GPIO16
- Yellow LED output: GPIO17
- Red LED output: GPIO19
- Buzzer output: GPIO18
- Relay output: GPIO23

## Notes
- Keep the same logical signal names across zones A, B, and C even if wiring changes later.
- Confirm ESP32 boot strap and input-only pin restrictions before final hardware lock-in.
- GPIO34 is input-only, which fits the MQ-2 analog input placeholder.
