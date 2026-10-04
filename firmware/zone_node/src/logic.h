#pragma once

#include <Arduino.h>

#include "sensors.h"

// Deterministic local warning logic lives here.
// This module evaluates sensor inputs and manages state transition timing only.

namespace app {

enum class ZoneState {
  kNormal,
  kWarning,
  kAlarm,
  kLinkage,
};

enum LogicReasonFlag : uint32_t {
  kReasonNone = 0,
  kReasonMq2Warning = 1u << 0,
  kReasonTemperatureWarning = 1u << 1,
  kReasonMq2Alarm = 1u << 2,
  kReasonTemperatureAlarm = 1u << 3,
  kReasonFlameStableDetected = 1u << 4,
  kReasonCombinedMq2AndTemperatureWarning = 1u << 5,
  kReasonAlarmPersisted = 1u << 6,
  kReasonLinkageHoldActive = 1u << 7,
};

struct LogicResult {
  ZoneState state = ZoneState::kNormal;
  ZoneState previousState = ZoneState::kNormal;
  bool stateChangedThisCycle = false;
  bool relayShouldActivate = false;
  bool buzzerShouldActivate = false;
  unsigned long lastStateChangeMs = 0;
  unsigned long alarmConditionSinceMs = 0;
  unsigned long linkageActivatedMs = 0;
  uint32_t reasonFlags = kReasonNone;
  const char* activeReason = "all monitored conditions below warning thresholds";
  const char* transitionReason = "initial state";
};

class WarningStateMachine {
 public:
  void begin(unsigned long nowMs = 0);
  const LogicResult& update(const SensorReadings& readings, unsigned long nowMs);
  const LogicResult& current() const;

 private:
  LogicResult result_{};
  unsigned long warningConditionSinceMs_ = 0;
  unsigned long alarmConditionSinceMs_ = 0;
  unsigned long linkageActivatedMs_ = 0;
};

const char* zoneStateToString(ZoneState state);
const char* systemStateToString(ZoneState state);

}  // namespace app
