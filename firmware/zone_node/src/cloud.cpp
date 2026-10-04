#include "cloud.h"

#include <ArduinoJson.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

namespace app {

CloudManager* CloudManager::activeInstance_ = nullptr;

CloudManager::CloudManager() : mqttClient_(wifiClient_) {}

void CloudManager::begin() {
  activeInstance_ = this;
  updateTopicBuffers();

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  mqttClient_.setServer(kAliyunBroker, kAliyunPort);
  mqttClient_.setKeepAlive(kMqttKeepAliveSeconds);
  mqttClient_.setBufferSize(kMqttPacketBufferSize);
  mqttClient_.setCallback(mqttCallbackTrampoline);

  Serial.println("[cloud] scaffold initialized");
  Serial.print("[cloud] zone profile: ");
  Serial.println(kZoneId);
  Serial.print("[cloud] uplink topic: ");
  Serial.println(upTopic_);
  Serial.print("[cloud] downlink topic: ");
  Serial.println(downTopic_);

  if (!configLooksReady()) {
    Serial.println("[cloud] placeholders still configured in config_local.h; Wi-Fi and MQTT connect attempts are skipped");
    configWarningPrinted_ = true;
  }
}

void CloudManager::update(unsigned long nowMs, const SensorReadings& readings,
                          const LogicResult& logicResult) {
  expireTemporaryOverrides(nowMs);

  if (!configLooksReady()) {
    if (!configWarningPrinted_) {
      Serial.println("[cloud] placeholders still configured in config_local.h; Wi-Fi and MQTT connect attempts are skipped");
      configWarningPrinted_ = true;
    }
    return;
  }

  ensureWifiConnected(nowMs);
  if (isWifiConnected()) {
    ensureMqttConnected(nowMs);
  }

  if (isMqttConnected()) {
    mqttClient_.loop();
    publishTelemetry(nowMs, readings, logicResult, publishRequested_);
  }

  expireTemporaryOverrides(nowMs);
}

bool CloudManager::isWifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool CloudManager::isMqttConnected() {
  return mqttClient_.connected();
}

bool CloudManager::hasManualRelayOverride() const {
  return manualRelayMode_ != ManualRelayMode::kFollowLocal;
}

bool CloudManager::manualRelayOverrideValue() const {
  return manualRelayMode_ == ManualRelayMode::kForceOn;
}

bool CloudManager::isBuzzerMuted() const {
  return buzzerMuted_;
}

void CloudManager::requestImmediatePublish() {
  publishRequested_ = true;
}

void CloudManager::mqttCallbackTrampoline(char* topic, uint8_t* payload, unsigned int length) {
  if (activeInstance_ == nullptr) {
    return;
  }

  activeInstance_->handleMqttMessage(topic, payload, length, millis());
}

void CloudManager::ensureWifiConnected(unsigned long nowMs) {
  if (isWifiConnected()) {
    return;
  }

  if (lastWifiAttemptMs_ != 0 && nowMs - lastWifiAttemptMs_ < kWifiReconnectIntervalMs) {
    return;
  }

  lastWifiAttemptMs_ = nowMs;
  Serial.print("[cloud] Wi-Fi connect attempt SSID=");
  Serial.println(kWifiSsid);
  WiFi.begin(kWifiSsid, kWifiPassword);
}

void CloudManager::ensureMqttConnected(unsigned long nowMs) {
  if (mqttClient_.connected()) {
    return;
  }

  if (lastMqttAttemptMs_ != 0 && nowMs - lastMqttAttemptMs_ < kMqttReconnectIntervalMs) {
    return;
  }

  lastMqttAttemptMs_ = nowMs;
  updateTopicBuffers();

  Serial.print("[cloud] MQTT connect attempt broker=");
  Serial.print(kAliyunBroker);
  Serial.print(":");
  Serial.println(kAliyunPort);

  if (mqttClient_.connect(kAliyunMqttClientId, kAliyunMqttUsername, kAliyunMqttPassword)) {
    Serial.println("[cloud] MQTT connected");
    if (mqttClient_.subscribe(downTopic_)) {
      Serial.print("[cloud] subscribed downlink topic ");
      Serial.println(downTopic_);
    } else {
      Serial.println("[cloud] downlink subscribe failed");
    }
    publishRequested_ = true;
    return;
  }

  Serial.print("[cloud] MQTT connect failed rc=");
  Serial.println(mqttClient_.state());
}

void CloudManager::expireTemporaryOverrides(unsigned long nowMs) {
  if (manualRelayMode_ != ManualRelayMode::kFollowLocal && manualRelayOverrideUntilMs_ != 0 &&
      nowMs >= manualRelayOverrideUntilMs_) {
    manualRelayMode_ = ManualRelayMode::kFollowLocal;
    manualRelayOverrideUntilMs_ = 0;
    Serial.println("[cloud] manual relay override expired");
  }

  if (buzzerMuted_ && buzzerMuteUntilMs_ != 0 && nowMs >= buzzerMuteUntilMs_) {
    buzzerMuted_ = false;
    buzzerMuteUntilMs_ = 0;
    Serial.println("[cloud] buzzer mute expired");
  }
}

void CloudManager::updateTopicBuffers() {
  snprintf(upTopic_, sizeof(upTopic_), "/%s/%s%s", kAliyunProductKey, kAliyunDeviceName,
           kCloudUpTopicSuffix);
  snprintf(downTopic_, sizeof(downTopic_), "/%s/%s%s", kAliyunProductKey, kAliyunDeviceName,
           kCloudDownTopicSuffix);
}

void CloudManager::handleMqttMessage(char* topic, const uint8_t* payload, unsigned int length,
                                     unsigned long nowMs) {
  StaticJsonDocument<256> doc;
  const DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.print("[cloud] downlink JSON parse failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char* command = doc["command"] | "";
  const char* value = doc["value"] | "";

  Serial.print("[cloud] downlink topic=");
  Serial.print(topic);
  Serial.print(" command=");
  Serial.print(command);
  Serial.print(" value=");
  Serial.println(value);

  if (strcmp(command, "relay") == 0) {
    applyRelayCommand(value, nowMs);
    return;
  }

  if (strcmp(command, "buzzer") == 0) {
    applyBuzzerCommand(value, nowMs);
    return;
  }

  if (strcmp(command, "request_status_publish") == 0) {
    publishRequested_ = true;
    Serial.println("[cloud] immediate telemetry publish requested");
    return;
  }

  Serial.println("[cloud] unsupported downlink command");
}

void CloudManager::publishTelemetry(unsigned long nowMs, const SensorReadings& readings,
                                    const LogicResult& logicResult, bool forcePublish) {
  const bool publishDue = lastTelemetryPublishMs_ == 0 ||
                          nowMs - lastTelemetryPublishMs_ >= kTelemetryIntervalMs;
  if (!forcePublish && !publishDue) {
    return;
  }

  StaticJsonDocument<512> doc;
  doc["zone"] = kZoneId;
  doc["state"] = zoneStateToString(logicResult.state);
  doc["mq2Raw"] = readings.mq2Raw;
  doc["mq2Processed"] = readings.mq2ProcessedValid ? readings.mq2Processed : 0.0f;
  doc["temperatureC"] = readings.dhtHasUsableValues ? readings.temperatureC : 0.0f;
  doc["humidityPercent"] = readings.dhtHasUsableValues ? readings.humidityPercent : 0.0f;
  doc["flameStableDetected"] = readings.flameStableDetected;
  doc["buzzerActive"] = computedBuzzerActive(logicResult);
  doc["relayActive"] = computedRelayActive(logicResult);
  doc["demoModeActive"] = readings.demoModeActive;
  doc["activeReason"] = logicResult.activeReason;
  doc["uptimeMs"] = nowMs;

  char payload[kMqttPacketBufferSize];
  const size_t payloadLength = serializeJson(doc, payload, sizeof(payload));
  if (payloadLength == 0) {
    Serial.println("[cloud] telemetry serialization failed");
    return;
  }

  if (mqttClient_.publish(upTopic_, payload)) {
    lastTelemetryPublishMs_ = nowMs;
    publishRequested_ = false;
    Serial.print("[cloud] telemetry published to ");
    Serial.println(upTopic_);
  } else {
    Serial.println("[cloud] telemetry publish failed");
  }
}

void CloudManager::applyRelayCommand(const char* value, unsigned long nowMs) {
  if (strcmp(value, "on") == 0) {
    manualRelayMode_ = ManualRelayMode::kForceOn;
    manualRelayOverrideUntilMs_ = nowMs + kCloudManualRelayOverrideDurationMs;
    Serial.println("[cloud] manual relay override set to ON");
    return;
  }

  if (strcmp(value, "off") == 0) {
    manualRelayMode_ = ManualRelayMode::kForceOff;
    manualRelayOverrideUntilMs_ = nowMs + kCloudManualRelayOverrideDurationMs;
    Serial.println("[cloud] manual relay override set to OFF");
    return;
  }

  Serial.println("[cloud] unsupported relay command value");
}

void CloudManager::applyBuzzerCommand(const char* value, unsigned long nowMs) {
  if (strcmp(value, "mute") == 0) {
    buzzerMuted_ = true;
    buzzerMuteUntilMs_ = nowMs + kCloudBuzzerMuteDurationMs;
    Serial.println("[cloud] buzzer mute enabled");
    return;
  }

  if (strcmp(value, "unmute") == 0) {
    buzzerMuted_ = false;
    buzzerMuteUntilMs_ = 0;
    Serial.println("[cloud] buzzer mute cleared");
    return;
  }

  Serial.println("[cloud] unsupported buzzer command value");
}

bool CloudManager::configLooksReady() const {
  return !valueLooksPlaceholder(kWifiSsid) && !valueLooksPlaceholder(kWifiPassword) &&
         !valueLooksPlaceholder(kAliyunBroker) && !valueLooksPlaceholder(kAliyunProductKey) &&
         !valueLooksPlaceholder(kAliyunDeviceName) && !valueLooksPlaceholder(kAliyunDeviceSecret) &&
         !valueLooksPlaceholder(kAliyunMqttClientId) &&
         !valueLooksPlaceholder(kAliyunMqttUsername) &&
         !valueLooksPlaceholder(kAliyunMqttPassword);
}

bool CloudManager::valueLooksPlaceholder(const char* value) const {
  return value == nullptr || value[0] == '\0' || strncmp(value, "YOUR_", 5) == 0;
}

bool CloudManager::computedRelayActive(const LogicResult& logicResult) const {
  bool relayActive = logicResult.relayShouldActivate;

  if (manualRelayMode_ == ManualRelayMode::kForceOn) {
    relayActive = true;
  } else if (manualRelayMode_ == ManualRelayMode::kForceOff) {
    relayActive = false;
  }

  return relayActive;
}

bool CloudManager::computedBuzzerActive(const LogicResult& logicResult) const {
  return logicResult.buzzerShouldActivate && !buzzerMuted_;
}

}  // namespace app
