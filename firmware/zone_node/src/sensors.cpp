#include "sensors.h"

#include <Arduino.h>
#include <math.h>

#include "pins.h"

namespace app {

SensorManager::SensorManager() : dht_(pins::kDht11Data, DHT11) {}

void SensorManager::begin() {
  analogReadResolution(12);
  pinMode(pins::kMq2AnalogInput, INPUT);
  pinMode(pins::kFlameDigitalInput, INPUT);
  dht_.begin();
}

const SensorReadings& SensorManager::update(unsigned long nowMs) {
  latestReadings_.mq2SampledThisCycle = false;
  latestReadings_.dhtReadFreshThisCycle = false;
  latestReadings_.dhtReadAttemptedThisCycle = false;
  latestReadings_.dhtLastReadFailed = false;
  latestReadings_.dhtUsingCachedValues = latestReadings_.dhtHasUsableValues;
  latestReadings_.flameSampledThisCycle = false;
  latestReadings_.flameStableChangedThisCycle = false;
  latestReadings_.demoModeActive = false;
  latestReadings_.mq2OverrideActive = false;
  latestReadings_.tempOverrideActive = false;
  latestReadings_.flameOverrideActive = false;

  updateMq2(nowMs);
  updateDht11(nowMs);
  updateFlame(nowMs);
  refreshCompatibilityFields();
  return latestReadings_;
}

const SensorReadings& SensorManager::latest() const {
  return latestReadings_;
}

void SensorManager::updateMq2(unsigned long nowMs) {
  const bool shouldSample = (lastMq2SampleAttemptMs_ == 0) ||
                            (nowMs - lastMq2SampleAttemptMs_ >= kMq2SampleIntervalMs);
  if (!shouldSample) {
    return;
  }

  lastMq2SampleAttemptMs_ = nowMs;
  const int rawValue = analogRead(pins::kMq2AnalogInput);

  latestReadings_.mq2Raw = rawValue;
  latestReadings_.mq2ReadOk = true;
  latestReadings_.mq2SampledThisCycle = true;
  latestReadings_.lastMq2ReadMs = nowMs;

  if (mq2SampleCount_ < kMq2FilterWindowSize) {
    mq2SampleSum_ += rawValue;
    mq2Samples_[mq2SampleIndex_] = rawValue;
    ++mq2SampleCount_;
  } else {
    mq2SampleSum_ -= mq2Samples_[mq2SampleIndex_];
    mq2Samples_[mq2SampleIndex_] = rawValue;
    mq2SampleSum_ += rawValue;
  }

  mq2SampleIndex_ = (mq2SampleIndex_ + 1) % kMq2FilterWindowSize;
  latestReadings_.mq2Processed = static_cast<float>(mq2SampleSum_) / mq2SampleCount_;
  latestReadings_.mq2ProcessedValid = mq2SampleCount_ > 0;
}

void SensorManager::updateDht11(unsigned long nowMs) {
  const bool shouldSample = (lastDhtSampleAttemptMs_ == 0) ||
                            (nowMs - lastDhtSampleAttemptMs_ >= kDhtReadIntervalMs);
  if (!shouldSample) {
    return;
  }

  lastDhtSampleAttemptMs_ = nowMs;
  latestReadings_.dhtReadAttemptedThisCycle = true;
  latestReadings_.lastDhtAttemptMs = nowMs;

  const float humidity = dht_.readHumidity();
  const float temperatureC = dht_.readTemperature();
  if (isnan(humidity) || isnan(temperatureC)) {
    latestReadings_.dhtLastReadFailed = true;
    latestReadings_.dhtUsingCachedValues = latestReadings_.dhtHasUsableValues;
    return;
  }

  latestReadings_.humidityPercent = humidity;
  latestReadings_.temperatureC = temperatureC;
  latestReadings_.dhtHasUsableValues = true;
  latestReadings_.dhtReadFreshThisCycle = true;
  latestReadings_.dhtUsingCachedValues = false;
  latestReadings_.lastDhtReadMs = nowMs;
}

void SensorManager::updateFlame(unsigned long nowMs) {
  const bool shouldSample = (lastFlameSampleAttemptMs_ == 0) ||
                            (nowMs - lastFlameSampleAttemptMs_ >= kFlameSampleIntervalMs);
  if (!shouldSample) {
    return;
  }

  lastFlameSampleAttemptMs_ = nowMs;
  const bool rawDetected = digitalRead(pins::kFlameDigitalInput) == LOW;

  latestReadings_.flameRawDetected = rawDetected;
  latestReadings_.flameReadOk = true;
  latestReadings_.flameSampledThisCycle = true;
  latestReadings_.lastFlameSampleMs = nowMs;

  if (rawDetected == latestReadings_.flameStableDetected) {
    pendingFlameState_ = rawDetected;
    pendingFlameSampleCount_ = 0;
    return;
  }

  if (rawDetected != pendingFlameState_) {
    pendingFlameState_ = rawDetected;
    pendingFlameSampleCount_ = 1;
  } else {
    ++pendingFlameSampleCount_;
  }

  if (pendingFlameSampleCount_ >= kFlameDebounceCount) {
    latestReadings_.flameStableDetected = pendingFlameState_;
    latestReadings_.lastFlameStableChangeMs = nowMs;
    latestReadings_.flameStableChangedThisCycle = true;
    pendingFlameSampleCount_ = 0;
  }
}

void SensorManager::refreshCompatibilityFields() {
  latestReadings_.smokeRaw = latestReadings_.mq2Raw;
  latestReadings_.flameDetected = latestReadings_.flameStableDetected;
  latestReadings_.dhtReadOk = latestReadings_.dhtHasUsableValues;
  latestReadings_.lastFlameReadMs = latestReadings_.lastFlameSampleMs;
}

SensorReadings buildEffectiveReadings(const SensorReadings& baseReadings, const DemoOverrides& overrides,
                                      unsigned long nowMs) {
  SensorReadings effectiveReadings = baseReadings;

  if (!overrides.enabled) {
    return effectiveReadings;
  }

  effectiveReadings.demoModeActive = true;

  if (overrides.mq2OverrideSet) {
    effectiveReadings.mq2Raw = overrides.mq2Value;
    effectiveReadings.mq2Processed = static_cast<float>(overrides.mq2Value);
    effectiveReadings.mq2ReadOk = true;
    effectiveReadings.mq2ProcessedValid = true;
    effectiveReadings.mq2OverrideActive = true;
    effectiveReadings.lastMq2ReadMs = nowMs;
  }

  if (overrides.tempOverrideSet) {
    effectiveReadings.temperatureC = overrides.temperatureValueC;
    effectiveReadings.dhtHasUsableValues = true;
    effectiveReadings.dhtUsingCachedValues = false;
    effectiveReadings.dhtReadFreshThisCycle = false;
    effectiveReadings.dhtLastReadFailed = false;
    effectiveReadings.tempOverrideActive = true;
    effectiveReadings.lastDhtReadMs = nowMs;
  }

  if (overrides.flameOverrideSet) {
    effectiveReadings.flameRawDetected = overrides.flameDetected;
    effectiveReadings.flameStableDetected = overrides.flameDetected;
    effectiveReadings.flameReadOk = true;
    effectiveReadings.flameOverrideActive = true;
    effectiveReadings.lastFlameSampleMs = nowMs;
  }

  effectiveReadings.smokeRaw = effectiveReadings.mq2Raw;
  effectiveReadings.flameDetected = effectiveReadings.flameStableDetected;
  effectiveReadings.dhtReadOk = effectiveReadings.dhtHasUsableValues;
  effectiveReadings.lastFlameReadMs = effectiveReadings.lastFlameSampleMs;

  return effectiveReadings;
}

bool demoOverridesConfigured(const DemoOverrides& overrides) {
  return overrides.mq2OverrideSet || overrides.tempOverrideSet || overrides.flameOverrideSet;
}

const char* systemStateToString(SystemState state) {
  switch (state) {
    case SystemState::kNormal:
      return "normal";
    case SystemState::kWarning:
      return "warning";
    case SystemState::kAlarm:
      return "alarm";
    default:
      return "unknown";
  }
}

}  // namespace app
