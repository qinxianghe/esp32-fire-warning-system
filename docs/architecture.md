# System Architecture

## Overview
This project models a multi-zone library fire early warning system with three ESP32-based zone nodes: Zone A, Zone B, and Zone C. Each node uses the same shared firmware source tree. Deployment differences are limited to local configuration values such as zone identity, Wi-Fi settings, and Aliyun per-device credentials.

## Shared Firmware Structure
The repository keeps one PlatformIO firmware project under `firmware/zone_node/`.
- `config.h` contains shared constants, timing values, thresholds, and topic suffixes.
- `config_local.h` is a local untracked file for Wi-Fi credentials, Aliyun broker settings, and per-device zone identity.
- `config_local.example.h` documents the expected local configuration layout safely with placeholders.
- PlatformIO environments select Zone A, Zone B, or Zone C through build flags while reusing the same code.

## Zone Node Responsibilities
Each zone node contains:
- MQ-2 smoke sensor input
- DHT11 temperature and humidity input
- IR flame sensor input
- Green LED for normal state
- Yellow LED for warning state
- Red LED for alarm state
- Buzzer output for local audible alert
- One relay output for linkage demonstration

The firmware keeps sensor acquisition, rule evaluation, actuator control, demo overrides, and cloud communication separated into modules.

## Local Deterministic Logic
Warning and alarm decisions are made locally on the ESP32 using deterministic rule-based logic. The node must keep reading sensors, evaluating rules, and driving outputs even if Wi-Fi or MQTT is unavailable. This preserves local fire warning behavior independently from cloud health.

## Connectivity and Telemetry
Each ESP32 zone node connects to local Wi-Fi and then to Aliyun IoT over MQTT when local credentials are present. Telemetry messages should include:
- zone identity
- sensor readings
- current local state
- relay linkage state when relevant
- health or connectivity status when useful for debugging

Real device credentials, endpoints, and product metadata remain only in the local untracked configuration file.

## Downlink Commands
Cloud downlink messages are intended for explicit command-style interactions such as:
- temporary relay on or off for demonstration
- temporary buzzer mute or unmute
- request a status snapshot publish

Downlink handling remains explicit, temporary, and logged clearly over the serial console.

## Local Actuation
Each node must be able to actuate local outputs immediately, without waiting for cloud approval:
- LEDs indicate normal, warning, or alarm status
- buzzer indicates local alert escalation unless temporarily muted by a cloud demo command
- relay indicates linkage state unless temporarily overridden by a cloud demo command

The local node remains safe and responsive even if Wi-Fi or MQTT is unavailable.
