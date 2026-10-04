#include "logic.h"

#include "config.h"

namespace app {
namespace {

constexpr const char* kReasonAllClear = "all monitored conditions below warning thresholds";
constexpr const char* kReasonMq2WarningText = "mq2Processed exceeded warning threshold";
constexpr const char* kReasonTemperatureWarningText = "temperature exceeded warning threshold";
constexpr const char* kReasonMq2AlarmText = "mq2Processed exceeded alarm threshold";
constexpr const char* kReasonTemperatureAlarmText = "temperature exceeded alarm threshold";
constexpr const char* kReasonFlameText = "flameStableDetected became true";
constexpr const char* kReasonCombinedWarningText =
    "mq2Processed warning and temperature warning were present together";
constexpr const char* kReasonAlarmPersistedText =
    "alarm persisted for the configured linkage delay";
constexpr const char* kReasonLinkageHoldText =
    "linkage relay hold remains active after alarm clearance";

uint32_t buildReasonFlags(const SensorReadings& readings) {
  uint32_t flags = kReasonNone;

  const bool mq2Warning = readings.mq2ProcessedValid &&
                          readings.mq2Processed >= static_cast<float>(kMq2WarningThreshold);
  const bool mq2Alarm = readings.mq2ProcessedValid &&
                        readings.mq2Processed >= static_cast<float>(kMq2AlarmThreshold);
  const bool temperatureWarning = readings.dhtHasUsableValues &&
                                  readings.temperatureC >= kTemperatureWarningThresholdC;
  const bool temperatureAlarm = readings.dhtHasUsableValues &&
                                readings.temperatureC >= kTemperatureAlarmThresholdC;

  if (mq2Warning) {
    flags |= kReasonMq2Warning;
  }
  if (temperatureWarning) {
    flags |= kReasonTemperatureWarning;
  }
  if (mq2Alarm) {
    flags |= kReasonMq2Alarm;
  }
  if (temperatureAlarm) {
    flags |= kReasonTemperatureAlarm;
  }
  if (readings.flameStableDetected) {
    flags |= kReasonFlameStableDetected;
  }
  if (mq2Warning && temperatureWarning) {
    flags |= kReasonCombinedMq2AndTemperatureWarning;
  }

  return flags;
}

bool warningConditionActive(uint32_t flags) {
  return (flags & kReasonMq2Warning) != 0 || (flags & kReasonTemperatureWarning) != 0;
}

bool alarmConditionActive(uint32_t flags) {
  return (flags & kReasonFlameStableDetected) != 0 || (flags & kReasonMq2Alarm) != 0 ||
         (flags & kReasonTemperatureAlarm) != 0 ||
         (flags & kReasonCombinedMq2AndTemperatureWarning) != 0;
}

const char* reasonFlagsToString(uint32_t flags) {
  if ((flags & kReasonLinkageHoldActive) != 0) {
    return kReasonLinkageHoldText;
  }
  if ((flags & kReasonAlarmPersisted) != 0) {
    return kReasonAlarmPersistedText;
  }
  if ((flags & kReasonFlameStableDetected) != 0) {
    return kReasonFlameText;
  }
  if ((flags & kReasonMq2Alarm) != 0) {
    return kReasonMq2AlarmText;
  }
  if ((flags & kReasonTemperatureAlarm) != 0) {
    return kReasonTemperatureAlarmText;
  }
  if ((flags & kReasonCombinedMq2AndTemperatureWarning) != 0) {
    return kReasonCombinedWarningText;
  }
  if ((flags & kReasonMq2Warning) != 0) {
    return kReasonMq2WarningText;
  }
  if ((flags & kReasonTemperatureWarning) != 0) {
    return kReasonTemperatureWarningText;
  }
  return kReasonAllClear;
}

void applyOutputFlags(LogicResult& result) {
  result.buzzerShouldActivate =
      result.state == ZoneState::kAlarm || result.state == ZoneState::kLinkage;
  result.relayShouldActivate = result.state == ZoneState::kLinkage;
}

}  // namespace

void WarningStateMachine::begin(unsigned long nowMs) {
  result_.state = ZoneState::kNormal;
  result_.previousState = ZoneState::kNormal;
  result_.stateChangedThisCycle = false;
  result_.relayShouldActivate = false;
  result_.buzzerShouldActivate = false;
  result_.lastStateChangeMs = nowMs;
  result_.alarmConditionSinceMs = 0;
  result_.linkageActivatedMs = 0;
  result_.reasonFlags = kReasonNone;
  result_.activeReason = kReasonAllClear;
  result_.transitionReason = "state machine initialized";
  warningConditionSinceMs_ = 0;
  alarmConditionSinceMs_ = 0;
  linkageActivatedMs_ = 0;
}

const LogicResult& WarningStateMachine::update(const SensorReadings& readings, unsigned long nowMs) {
  result_.stateChangedThisCycle = false;

  const uint32_t baseFlags = buildReasonFlags(readings);
  const bool warningActive = warningConditionActive(baseFlags);
  const bool alarmActive = alarmConditionActive(baseFlags);

  if (warningActive) {
    if (warningConditionSinceMs_ == 0) {
      warningConditionSinceMs_ = nowMs;
    }
  } else {
    warningConditionSinceMs_ = 0;
  }

  if (alarmActive) {
    if (alarmConditionSinceMs_ == 0) {
      alarmConditionSinceMs_ = nowMs;
    }
  } else {
    alarmConditionSinceMs_ = 0;
  }

  uint32_t activeFlags = baseFlags;
  ZoneState nextState = ZoneState::kNormal;

  const bool linkageHoldActive = result_.state == ZoneState::kLinkage && !alarmActive &&
                                 linkageActivatedMs_ != 0 &&
                                 (nowMs - linkageActivatedMs_ < kRelayLinkageDurationMs);

  if (linkageHoldActive) {
    nextState = ZoneState::kLinkage;
    activeFlags |= kReasonLinkageHoldActive;
  } else if (alarmActive) {
    nextState = ZoneState::kAlarm;
    if (alarmConditionSinceMs_ != 0 &&
        nowMs - alarmConditionSinceMs_ >= kAlarmPersistenceDurationMs) {
      nextState = ZoneState::kLinkage;
      activeFlags |= kReasonAlarmPersisted;
    }
  } else if (warningActive && warningConditionSinceMs_ != 0 &&
             nowMs - warningConditionSinceMs_ >= kWarningPersistenceDurationMs) {
    nextState = ZoneState::kWarning;
  }

  const ZoneState previousState = result_.state;
  result_.previousState = previousState;
  result_.state = nextState;
  result_.reasonFlags = activeFlags;
  result_.activeReason = reasonFlagsToString(activeFlags);

  if (nextState == ZoneState::kLinkage && previousState != ZoneState::kLinkage) {
    linkageActivatedMs_ = nowMs;
  } else if (nextState != ZoneState::kLinkage && !linkageHoldActive && !alarmActive) {
    linkageActivatedMs_ = 0;
  }

  result_.alarmConditionSinceMs = alarmConditionSinceMs_;
  result_.linkageActivatedMs = linkageActivatedMs_;

  if (nextState != previousState) {
    result_.stateChangedThisCycle = true;
    result_.lastStateChangeMs = nowMs;
    result_.transitionReason = result_.activeReason;
  }

  applyOutputFlags(result_);
  return result_;
}

const LogicResult& WarningStateMachine::current() const {
  return result_;
}

const char* zoneStateToString(ZoneState state) {
  switch (state) {
    case ZoneState::kNormal:
      return "NORMAL";
    case ZoneState::kWarning:
      return "WARNING";
    case ZoneState::kAlarm:
      return "ALARM";
    case ZoneState::kLinkage:
      return "LINKAGE";
    default:
      return "UNKNOWN";
  }
}

const char* systemStateToString(ZoneState state) {
  return zoneStateToString(state);
}

}  // namespace app
