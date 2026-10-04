#pragma once

#include <stddef.h>
#include <stdint.h>

// Shared firmware configuration for one ESP32 zone node.
// Zone identity and private Wi-Fi or Aliyun values are loaded from a local config header.

#if __has_include("config_local.h")
#include "config_local.h"
#else
#include "config_local.example.h"
#endif

#if (defined(APP_ZONE_PROFILE_A) + defined(APP_ZONE_PROFILE_B) + defined(APP_ZONE_PROFILE_C)) > 1
#error "Select only one zone profile: APP_ZONE_PROFILE_A, APP_ZONE_PROFILE_B, or APP_ZONE_PROFILE_C."
#endif

#if (defined(APP_ZONE_PROFILE_A) + defined(APP_ZONE_PROFILE_B) + defined(APP_ZONE_PROFILE_C)) == 0
#error "No zone profile selected. Build with esp32-zone-a, esp32-zone-b, or esp32-zone-c."
#endif

#ifndef APP_ZONE_ID
#error "APP_ZONE_ID must be defined by config_local.h for the selected zone profile."
#endif

#ifndef APP_WIFI_SSID
#error "APP_WIFI_SSID must be defined in config_local.h."
#endif

#ifndef APP_WIFI_PASSWORD
#error "APP_WIFI_PASSWORD must be defined in config_local.h."
#endif

#ifndef APP_ALIYUN_PRODUCT_KEY
#error "APP_ALIYUN_PRODUCT_KEY must be defined in config_local.h."
#endif

#ifndef APP_ALIYUN_DEVICE_NAME
#error "APP_ALIYUN_DEVICE_NAME must be defined in config_local.h for the selected zone profile."
#endif

#ifndef APP_ALIYUN_DEVICE_SECRET
#error "APP_ALIYUN_DEVICE_SECRET must be defined in config_local.h for the selected zone profile."
#endif

#ifndef APP_ALIYUN_BROKER
#error "APP_ALIYUN_BROKER must be defined in config_local.h."
#endif

#ifndef APP_ALIYUN_PORT
#error "APP_ALIYUN_PORT must be defined in config_local.h."
#endif

#ifndef APP_ALIYUN_MQTT_CLIENT_ID
#error "APP_ALIYUN_MQTT_CLIENT_ID must be defined in config_local.h for the selected zone profile."
#endif

#ifndef APP_ALIYUN_MQTT_USERNAME
#error "APP_ALIYUN_MQTT_USERNAME must be defined in config_local.h for the selected zone profile."
#endif

#ifndef APP_ALIYUN_MQTT_PASSWORD
#error "APP_ALIYUN_MQTT_PASSWORD must be defined in config_local.h for the selected zone profile."
#endif

namespace app {

constexpr bool kDebugLoggingEnabled = true;                 // Enables verbose serial logging and the serial debug command interface.
constexpr bool kDemoModeEnabled = false;                    // Default boot state for demo overrides; keep false for normal real-sensor behavior.

constexpr char kZoneId[] = APP_ZONE_ID;                     // Logical zone identifier selected from the local zone profile.

constexpr char kWifiSsid[] = APP_WIFI_SSID;                 // Local Wi-Fi SSID loaded from local configuration.
constexpr char kWifiPassword[] = APP_WIFI_PASSWORD;         // Local Wi-Fi password loaded from local configuration.
constexpr unsigned long kWifiReconnectIntervalMs = 10000;   // Interval between Wi-Fi connection attempts when disconnected.

constexpr char kAliyunProductKey[] = APP_ALIYUN_PRODUCT_KEY;       // Aliyun product key loaded from local configuration.
constexpr char kAliyunDeviceName[] = APP_ALIYUN_DEVICE_NAME;       // Aliyun device name for the selected zone node.
constexpr char kAliyunDeviceSecret[] = APP_ALIYUN_DEVICE_SECRET;   // Aliyun device secret for the selected zone node.
constexpr char kAliyunBroker[] = APP_ALIYUN_BROKER;                // Aliyun MQTT broker endpoint loaded from local configuration.
constexpr int kAliyunPort = APP_ALIYUN_PORT;                       // MQTT broker port for initial bring-up.
constexpr char kAliyunMqttClientId[] = APP_ALIYUN_MQTT_CLIENT_ID;  // MQTT client ID loaded from local configuration.
constexpr char kAliyunMqttUsername[] = APP_ALIYUN_MQTT_USERNAME;   // MQTT username loaded from local configuration.
constexpr char kAliyunMqttPassword[] = APP_ALIYUN_MQTT_PASSWORD;   // MQTT password or signature loaded from local configuration.
constexpr unsigned long kMqttReconnectIntervalMs = 5000;           // Interval between MQTT reconnect attempts when disconnected.
constexpr uint16_t kMqttKeepAliveSeconds = 60;                     // MQTT keepalive interval in seconds.
constexpr size_t kMqttPacketBufferSize = 512;                      // Buffer size for MQTT payload serialization and parsing.
constexpr char kCloudUpTopicSuffix[] = "/user/fire/up";          // Uplink topic suffix appended to /productKey/deviceName.
constexpr char kCloudDownTopicSuffix[] = "/user/fire/down";      // Downlink topic suffix appended to /productKey/deviceName.
constexpr unsigned long kCloudManualRelayOverrideDurationMs = 30000;  // Duration of a temporary manual relay override from cloud commands.
constexpr unsigned long kCloudBuzzerMuteDurationMs = 30000;           // Duration of a temporary buzzer mute from cloud commands.

constexpr unsigned long kSerialBaudRate = 115200;                // UART baud rate for serial debug logs.
constexpr size_t kSerialCommandBufferSize = 96;                  // Maximum length of one serial debug command line.
constexpr unsigned long kTelemetryIntervalMs = 5000;             // Interval between telemetry publishes while MQTT is connected.
constexpr unsigned long kSensorLogIntervalMs = 2000;             // Interval between serial sensor status prints during local self-test runs.
constexpr unsigned long kStateLogIntervalMs = 3000;              // Interval between periodic local state snapshot logs.
constexpr unsigned long kWarningPersistenceDurationMs = 3000;    // Time a non-critical warning condition must remain active before entering WARNING.
constexpr unsigned long kDhtReadIntervalMs = 2000;               // Interval between DHT11 read attempts.
constexpr unsigned long kMq2SampleIntervalMs = 1000;             // Interval between MQ-2 smoke sensor samples.
constexpr int kMq2FilterWindowSize = 8;                          // Number of recent MQ-2 samples used in the rolling average filter.
constexpr unsigned long kFlameSampleIntervalMs = 50;             // Fixed sample interval for deterministic flame sensor debouncing.
constexpr unsigned long kSelfTestLedStepDurationMs = 1000;       // Time to hold each LED on during the startup self-test.
constexpr unsigned long kSelfTestBuzzerOnDurationMs = 200;       // Time to hold the buzzer on for each startup beep.
constexpr unsigned long kSelfTestBuzzerOffDurationMs = 200;      // Pause between startup buzzer beeps.
constexpr unsigned long kSelfTestRelayDurationMs = 1000;         // Time to hold the relay on during the startup self-test.
constexpr int kFlameDebounceCount = 3;                           // Consecutive sampled flame states required before accepting a stable flame change.

constexpr int kMq2WarningThreshold = 300;                        // Processed MQ-2 threshold for warning evaluation.
constexpr int kMq2AlarmThreshold = 500;                          // Processed MQ-2 threshold for direct alarm evaluation.
constexpr float kTemperatureWarningThresholdC = 45.0f;           // Temperature threshold for warning evaluation.
constexpr float kTemperatureAlarmThresholdC = 55.0f;             // Temperature threshold for direct alarm evaluation.

constexpr unsigned long kAlarmPersistenceDurationMs = 10000;     // Continuous ALARM time required before transitioning to LINKAGE.
constexpr unsigned long kRelayLinkageDurationMs = 15000;         // Minimum time to hold LINKAGE relay output after linkage activates.
constexpr bool kRelayEnabledByDefault = false;                   // Default relay enable state at boot before any local or cloud override.

// Backward-compatible aliases for the current scaffold.
constexpr unsigned long kTelemetryPublishIntervalMs = kTelemetryIntervalMs;
constexpr int kSmokeWarningThreshold = kMq2WarningThreshold;
constexpr int kSmokeAlarmThreshold = kMq2AlarmThreshold;
constexpr float kTemperatureWarningC = kTemperatureWarningThresholdC;
constexpr float kTemperatureAlarmC = kTemperatureAlarmThresholdC;

}  // namespace app
