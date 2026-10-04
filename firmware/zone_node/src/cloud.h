#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "logic.h"
#include "sensors.h"

// Wi-Fi and Aliyun MQTT scaffolding lives here.
// Local safety logic must keep working even when connectivity is unavailable.

namespace app {

class CloudManager {
 public:
  CloudManager();

  void begin();
  void update(unsigned long nowMs, const SensorReadings& readings, const LogicResult& logicResult);

  bool isWifiConnected();
  bool isMqttConnected();
  bool hasManualRelayOverride() const;
  bool manualRelayOverrideValue() const;
  bool isBuzzerMuted() const;
  void requestImmediatePublish();

 private:
  enum class ManualRelayMode : uint8_t {
    kFollowLocal,
    kForceOn,
    kForceOff,
  };

  static CloudManager* activeInstance_;
  static void mqttCallbackTrampoline(char* topic, uint8_t* payload, unsigned int length);

  void ensureWifiConnected(unsigned long nowMs);
  void ensureMqttConnected(unsigned long nowMs);
  void expireTemporaryOverrides(unsigned long nowMs);
  void updateTopicBuffers();
  void handleMqttMessage(char* topic, const uint8_t* payload, unsigned int length, unsigned long nowMs);
  void publishTelemetry(unsigned long nowMs, const SensorReadings& readings,
                        const LogicResult& logicResult, bool forcePublish);
  void applyRelayCommand(const char* value, unsigned long nowMs);
  void applyBuzzerCommand(const char* value, unsigned long nowMs);
  bool configLooksReady() const;
  bool valueLooksPlaceholder(const char* value) const;
  bool computedRelayActive(const LogicResult& logicResult) const;
  bool computedBuzzerActive(const LogicResult& logicResult) const;

  WiFiClient wifiClient_;
  PubSubClient mqttClient_;
  char upTopic_[128] = {};
  char downTopic_[128] = {};
  unsigned long lastWifiAttemptMs_ = 0;
  unsigned long lastMqttAttemptMs_ = 0;
  unsigned long lastTelemetryPublishMs_ = 0;
  bool publishRequested_ = false;
  bool configWarningPrinted_ = false;
  ManualRelayMode manualRelayMode_ = ManualRelayMode::kFollowLocal;
  unsigned long manualRelayOverrideUntilMs_ = 0;
  bool buzzerMuted_ = false;
  unsigned long buzzerMuteUntilMs_ = 0;
};

}  // namespace app
