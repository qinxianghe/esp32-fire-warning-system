#pragma once

#include <Arduino.h>
#include <DHT.h>

#include "config.h"

// Sensor setup, timing, filtering, cached sensor values, and demo override helpers live here.
// This module does not decide warning states or drive actuators.

namespace app {

enum class SystemState {
  kNormal,
  kWarning,
  kAlarm,
};

struct SensorReadings {
  // MQ-2 raw and filtered values.
  int mq2Raw = 0;
  float mq2Processed = 0.0f;
  bool mq2ReadOk = false;
  bool mq2ProcessedValid = false;
  bool mq2SampledThisCycle = false;

  // Latest usable DHT11 values plus freshness state.
  float temperatureC = 0.0f;
  float humidityPercent = 0.0f;
  bool dhtHasUsableValues = false;
  bool dhtReadFreshThisCycle = false;
  bool dhtUsingCachedValues = false;
  bool dhtReadAttemptedThisCycle = false;
  bool dhtLastReadFailed = false;

  // Raw flame input and debounced stable flame state.
  bool flameRawDetected = false;
  bool flameStableDetected = false;
  bool flameReadOk = false;
  bool flameSampledThisCycle = false;
  bool flameStableChangedThisCycle = false;

  // Demo override markers for effective readings.
  bool demoModeActive = false;
  bool mq2OverrideActive = false;
  bool tempOverrideActive = false;
  bool flameOverrideActive = false;

  // Timestamps for recent successful reads and state transitions.
  unsigned long lastMq2ReadMs = 0;
  unsigned long lastDhtReadMs = 0;
  unsigned long lastDhtAttemptMs = 0;
  unsigned long lastFlameSampleMs = 0;
  unsigned long lastFlameStableChangeMs = 0;

  // Backward-compatible aliases for existing logic scaffolding.
  int smokeRaw = 0;
  bool flameDetected = false;
  bool dhtReadOk = false;
  unsigned long lastFlameReadMs = 0;
};

struct DemoOverrides {
  bool enabled = kDemoModeEnabled;
  bool mq2OverrideSet = false;
  int mq2Value = 0;
  bool tempOverrideSet = false;
  float temperatureValueC = 0.0f;
  bool flameOverrideSet = false;
  bool flameDetected = false;
};

class SensorManager {
 public:
  SensorManager();

  void begin();
  const SensorReadings& update(unsigned long nowMs);
  const SensorReadings& latest() const;

 private:
  void updateMq2(unsigned long nowMs);
  void updateDht11(unsigned long nowMs);
  void updateFlame(unsigned long nowMs);
  void refreshCompatibilityFields();

  DHT dht_;
  SensorReadings latestReadings_{};
  int mq2Samples_[kMq2FilterWindowSize] = {};
  int mq2SampleIndex_ = 0;
  int mq2SampleCount_ = 0;
  long mq2SampleSum_ = 0;
  unsigned long lastMq2SampleAttemptMs_ = 0;
  unsigned long lastDhtSampleAttemptMs_ = 0;
  unsigned long lastFlameSampleAttemptMs_ = 0;
  bool pendingFlameState_ = false;
  int pendingFlameSampleCount_ = 0;
};

SensorReadings buildEffectiveReadings(const SensorReadings& baseReadings, const DemoOverrides& overrides,
                                      unsigned long nowMs);
bool demoOverridesConfigured(const DemoOverrides& overrides);

const char* systemStateToString(SystemState state);

}  // namespace app
