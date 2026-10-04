#include <Arduino.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "actuators.h"
#include "cloud.h"
#include "config.h"
#include "logic.h"
#include "sensors.h"
#include "utils.h"

namespace {

app::SensorManager gSensors;
app::ActuatorManager gActuators;
app::WarningStateMachine gStateMachine;
app::CloudManager gCloud;
app::DemoOverrides gDemoOverrides;
app::SensorReadings gLastRealReadings;
app::SensorReadings gLastEffectiveReadings;
char gCommandBuffer[app::kSerialCommandBufferSize] = {};
size_t gCommandLength = 0;
unsigned long gLastSensorLogMs = 0;
unsigned long gLastStateLogMs = 0;

int compareIgnoreCase(const char* left, const char* right) {
  while (*left != '\0' && *right != '\0') {
    const int leftChar = tolower(static_cast<unsigned char>(*left));
    const int rightChar = tolower(static_cast<unsigned char>(*right));
    if (leftChar != rightChar) {
      return leftChar - rightChar;
    }
    ++left;
    ++right;
  }

  return tolower(static_cast<unsigned char>(*left)) -
         tolower(static_cast<unsigned char>(*right));
}

void holdForSelfTestStep(const char* label, void (app::ActuatorManager::*setter)(bool),
                         unsigned long durationMs) {
  Serial.print("[self-test] ");
  Serial.print(label);
  Serial.println(" ON");
  (gActuators.*setter)(true);
  delay(durationMs);  // Bounded setup-only timing for visible hardware verification.
  (gActuators.*setter)(false);
  Serial.print("[self-test] ");
  Serial.print(label);
  Serial.println(" OFF");
}

void runStartupSelfTest() {
  holdForSelfTestStep("green_led", &app::ActuatorManager::setGreenLed,
                      app::kSelfTestLedStepDurationMs);
  holdForSelfTestStep("yellow_led", &app::ActuatorManager::setYellowLed,
                      app::kSelfTestLedStepDurationMs);
  holdForSelfTestStep("red_led", &app::ActuatorManager::setRedLed,
                      app::kSelfTestLedStepDurationMs);

  for (int beepIndex = 0; beepIndex < 2; ++beepIndex) {
    Serial.print("[self-test] buzzer beep ");
    Serial.println(beepIndex + 1);
    gActuators.setBuzzer(true);
    delay(app::kSelfTestBuzzerOnDurationMs);
    gActuators.setBuzzer(false);
    delay(app::kSelfTestBuzzerOffDurationMs);
  }

  holdForSelfTestStep("relay", &app::ActuatorManager::setRelay, app::kSelfTestRelayDurationMs);
  gActuators.allOff();
  Serial.println("[self-test] complete");
}

bool effectiveRelayActive(const app::LogicResult& logicResult) {
  bool relayActive = logicResult.relayShouldActivate;
  if (gCloud.hasManualRelayOverride()) {
    relayActive = gCloud.manualRelayOverrideValue();
  }
  return relayActive;
}

bool effectiveBuzzerActive(const app::LogicResult& logicResult) {
  return logicResult.buzzerShouldActivate && !gCloud.isBuzzerMuted();
}

void applyOutputs(const app::LogicResult& logicResult) {
  gActuators.allOff();

  switch (logicResult.state) {
    case app::ZoneState::kNormal:
      gActuators.setGreenLed(true);
      break;
    case app::ZoneState::kWarning:
      gActuators.setYellowLed(true);
      break;
    case app::ZoneState::kAlarm:
      gActuators.setRedLed(true);
      break;
    case app::ZoneState::kLinkage:
      gActuators.setRedLed(true);
      break;
  }

  gActuators.setBuzzer(effectiveBuzzerActive(logicResult));
  gActuators.setRelay(effectiveRelayActive(logicResult));
}

void printCommandHelp() {
  Serial.println("[cmd] available commands:");
  Serial.println("[cmd]   help");
  Serial.println("[cmd]   status");
  Serial.println("[cmd]   selftest");
  Serial.println("[cmd]   demo on");
  Serial.println("[cmd]   demo off");
  Serial.println("[cmd]   set mq2 <value>");
  Serial.println("[cmd]   set temp <value>");
  Serial.println("[cmd]   set flame <0|1>");
  Serial.println("[cmd]   clear overrides");
}

void printOverrideStatus() {
  Serial.print("[demo] mode=");
  Serial.print(gDemoOverrides.enabled ? "on" : "off");
  Serial.print(" configured=");
  Serial.print(app::demoOverridesConfigured(gDemoOverrides) ? "yes" : "no");
  Serial.print(" mq2=");
  if (gDemoOverrides.mq2OverrideSet) {
    Serial.print(gDemoOverrides.mq2Value);
  } else {
    Serial.print("real");
  }
  Serial.print(" temp=");
  if (gDemoOverrides.tempOverrideSet) {
    Serial.print(gDemoOverrides.temperatureValueC, 1);
  } else {
    Serial.print("real");
  }
  Serial.print(" flame=");
  if (gDemoOverrides.flameOverrideSet) {
    Serial.println(gDemoOverrides.flameDetected ? "1" : "0");
  } else {
    Serial.println("real");
  }
}

void logSensorReadings(const app::SensorReadings& realReadings,
                       const app::SensorReadings& effectiveReadings,
                       const app::LogicResult& logicResult) {
  Serial.print("[sensor] zone=");
  Serial.print(app::kZoneId);
  Serial.print(" state=");
  Serial.print(app::zoneStateToString(logicResult.state));
  Serial.print(" demo=");
  Serial.print(effectiveReadings.demoModeActive ? "on" : "off");
  Serial.print(" wifi=");
  Serial.print(gCloud.isWifiConnected() ? "up" : "down");
  Serial.print(" mqtt=");
  Serial.print(gCloud.isMqttConnected() ? "up" : "down");

  Serial.print(" mq2_raw=");
  if (effectiveReadings.mq2ReadOk) {
    Serial.print(effectiveReadings.mq2Raw);
  } else {
    Serial.print("read_fail");
  }
  if (effectiveReadings.mq2OverrideActive) {
    Serial.print("(override real=");
    Serial.print(realReadings.mq2Raw);
    Serial.print(")");
  }

  Serial.print(" mq2_avg=");
  if (effectiveReadings.mq2ProcessedValid) {
    Serial.print(effectiveReadings.mq2Processed, 1);
  } else {
    Serial.print("read_fail");
  }

  Serial.print(" temp_c=");
  if (effectiveReadings.dhtHasUsableValues) {
    Serial.print(effectiveReadings.temperatureC, 1);
  } else {
    Serial.print("read_fail");
  }
  if (effectiveReadings.tempOverrideActive) {
    Serial.print("(override)");
  }

  Serial.print(" humidity_pct=");
  if (effectiveReadings.dhtHasUsableValues) {
    Serial.print(effectiveReadings.humidityPercent, 1);
  } else {
    Serial.print("read_fail");
  }

  Serial.print(" dht_source=");
  if (effectiveReadings.tempOverrideActive) {
    Serial.print("override");
  } else if (effectiveReadings.dhtReadFreshThisCycle) {
    Serial.print("fresh");
  } else if (effectiveReadings.dhtUsingCachedValues) {
    Serial.print("cached");
  } else {
    Serial.print("none");
  }

  Serial.print(" flame_raw=");
  if (effectiveReadings.flameReadOk) {
    Serial.print(effectiveReadings.flameRawDetected ? "true" : "false");
  } else {
    Serial.print("read_fail");
  }
  if (effectiveReadings.flameOverrideActive) {
    Serial.print("(override real=");
    Serial.print(realReadings.flameRawDetected ? "true" : "false");
    Serial.print(")");
  }

  Serial.print(" flame_stable=");
  Serial.println(effectiveReadings.flameStableDetected ? "true" : "false");

  if (realReadings.dhtReadAttemptedThisCycle && realReadings.dhtLastReadFailed &&
      !effectiveReadings.tempOverrideActive) {
    if (realReadings.dhtHasUsableValues) {
      Serial.println("[sensor] DHT11 read failure, using cached last-good values");
    } else {
      Serial.println("[sensor] DHT11 read failure, no cached values available");
    }
  }
}

void logStateTransition(const app::LogicResult& logicResult) {
  Serial.print("[state] transition ");
  Serial.print(app::zoneStateToString(logicResult.previousState));
  Serial.print(" -> ");
  Serial.print(app::zoneStateToString(logicResult.state));
  Serial.print(" because ");
  Serial.println(logicResult.transitionReason);
}

void logStateSnapshot(const app::LogicResult& logicResult, unsigned long nowMs) {
  Serial.print("[state] current=");
  Serial.print(app::zoneStateToString(logicResult.state));
  Serial.print(" reason=");
  Serial.print(logicResult.activeReason);
  Serial.print(" state_duration_ms=");
  Serial.print(nowMs - logicResult.lastStateChangeMs);
  Serial.print(" alarm_active_ms=");
  if (logicResult.alarmConditionSinceMs != 0) {
    Serial.print(nowMs - logicResult.alarmConditionSinceMs);
  } else {
    Serial.print(0);
  }
  Serial.print(" linkage_active_ms=");
  if (logicResult.linkageActivatedMs != 0) {
    Serial.print(nowMs - logicResult.linkageActivatedMs);
  } else {
    Serial.print(0);
  }
  Serial.print(" relay_override=");
  if (gCloud.hasManualRelayOverride()) {
    Serial.print(gCloud.manualRelayOverrideValue() ? "on" : "off");
  } else {
    Serial.print("none");
  }
  Serial.print(" buzzer_muted=");
  Serial.println(gCloud.isBuzzerMuted() ? "yes" : "no");
}

void printStatus(unsigned long nowMs) {
  const app::SensorReadings effectiveReadings =
      app::buildEffectiveReadings(gLastRealReadings, gDemoOverrides, nowMs);
  const app::LogicResult& logicResult = gStateMachine.current();

  printOverrideStatus();
  logSensorReadings(gLastRealReadings, effectiveReadings, logicResult);
  logStateSnapshot(logicResult, nowMs);
}

void clearOverrides() {
  gDemoOverrides.mq2OverrideSet = false;
  gDemoOverrides.tempOverrideSet = false;
  gDemoOverrides.flameOverrideSet = false;
}

void trimTrailingWhitespace(char* text) {
  size_t length = strlen(text);
  while (length > 0 && isspace(static_cast<unsigned char>(text[length - 1])) != 0) {
    text[length - 1] = '\0';
    --length;
  }
}

void handleCommand(char* commandLine, unsigned long nowMs) {
  trimTrailingWhitespace(commandLine);
  while (*commandLine != '\0' && isspace(static_cast<unsigned char>(*commandLine)) != 0) {
    ++commandLine;
  }

  if (*commandLine == '\0') {
    return;
  }

  char* context = nullptr;
  char* command = strtok_r(commandLine, " ", &context);
  if (command == nullptr) {
    return;
  }

  if (compareIgnoreCase(command, "help") == 0) {
    printCommandHelp();
    return;
  }

  if (compareIgnoreCase(command, "status") == 0) {
    printStatus(nowMs);
    return;
  }

  if (compareIgnoreCase(command, "selftest") == 0) {
    Serial.println("[cmd] rerunning startup self-test");
    runStartupSelfTest();
    applyOutputs(gStateMachine.current());
    return;
  }

  if (compareIgnoreCase(command, "demo") == 0) {
    char* mode = strtok_r(nullptr, " ", &context);
    if (mode == nullptr) {
      Serial.println("[cmd] usage: demo on|off");
      return;
    }

    if (compareIgnoreCase(mode, "on") == 0) {
      gDemoOverrides.enabled = true;
      Serial.println("[cmd] demo mode enabled");
      printOverrideStatus();
      return;
    }

    if (compareIgnoreCase(mode, "off") == 0) {
      gDemoOverrides.enabled = false;
      clearOverrides();
      Serial.println("[cmd] demo mode disabled and overrides cleared");
      printOverrideStatus();
      return;
    }

    Serial.println("[cmd] usage: demo on|off");
    return;
  }

  if (compareIgnoreCase(command, "set") == 0) {
    char* field = strtok_r(nullptr, " ", &context);
    char* value = strtok_r(nullptr, " ", &context);
    if (field == nullptr || value == nullptr) {
      Serial.println("[cmd] usage: set mq2 <value> | set temp <value> | set flame <0|1>");
      return;
    }

    if (!gDemoOverrides.enabled) {
      Serial.println("[cmd] demo mode is off; run 'demo on' before setting overrides");
      return;
    }

    if (compareIgnoreCase(field, "mq2") == 0) {
      gDemoOverrides.mq2OverrideSet = true;
      gDemoOverrides.mq2Value = atoi(value);
      Serial.print("[cmd] mq2 override set to ");
      Serial.println(gDemoOverrides.mq2Value);
      return;
    }

    if (compareIgnoreCase(field, "temp") == 0) {
      gDemoOverrides.tempOverrideSet = true;
      gDemoOverrides.temperatureValueC = static_cast<float>(atof(value));
      Serial.print("[cmd] temperature override set to ");
      Serial.println(gDemoOverrides.temperatureValueC, 1);
      return;
    }

    if (compareIgnoreCase(field, "flame") == 0) {
      if (strcmp(value, "0") != 0 && strcmp(value, "1") != 0) {
        Serial.println("[cmd] usage: set flame <0|1>");
        return;
      }
      gDemoOverrides.flameOverrideSet = true;
      gDemoOverrides.flameDetected = strcmp(value, "1") == 0;
      Serial.print("[cmd] flame override set to ");
      Serial.println(gDemoOverrides.flameDetected ? "1" : "0");
      return;
    }

    Serial.println("[cmd] usage: set mq2 <value> | set temp <value> | set flame <0|1>");
    return;
  }

  if (compareIgnoreCase(command, "clear") == 0) {
    char* target = strtok_r(nullptr, " ", &context);
    if (target != nullptr && compareIgnoreCase(target, "overrides") == 0) {
      clearOverrides();
      Serial.println("[cmd] demo overrides cleared");
      printOverrideStatus();
      return;
    }

    Serial.println("[cmd] usage: clear overrides");
    return;
  }

  Serial.println("[cmd] unknown command");
  printCommandHelp();
}

void processSerialCommands(unsigned long nowMs) {
  if (!app::kDebugLoggingEnabled) {
    return;
  }

  while (Serial.available() > 0) {
    const char incoming = static_cast<char>(Serial.read());
    if (incoming == '\r') {
      continue;
    }

    if (incoming == '\n') {
      gCommandBuffer[gCommandLength] = '\0';
      handleCommand(gCommandBuffer, nowMs);
      gCommandLength = 0;
      gCommandBuffer[0] = '\0';
      continue;
    }

    if (gCommandLength + 1 < app::kSerialCommandBufferSize) {
      gCommandBuffer[gCommandLength++] = incoming;
      gCommandBuffer[gCommandLength] = '\0';
    } else {
      gCommandLength = 0;
      gCommandBuffer[0] = '\0';
      Serial.println("[cmd] command too long");
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(app::kSerialBaudRate);
  delay(100);

  Serial.println("[boot] start");
  app::logBanner(app::kZoneId);

  gSensors.begin();
  Serial.println("[boot] sensors initialized");

  gActuators.begin();
  gActuators.allOff();
  Serial.println("[boot] actuators initialized");

  runStartupSelfTest();
  gStateMachine.begin(millis());
  gCloud.begin();
  applyOutputs(gStateMachine.current());

  if (app::kDebugLoggingEnabled) {
    Serial.println("[boot] serial debug commands enabled; type 'help' for commands");
  }
  printOverrideStatus();
  Serial.println("[boot] entering state-machine monitor loop");
}

void loop() {
  const unsigned long nowMs = millis();
  processSerialCommands(nowMs);

  gLastRealReadings = gSensors.update(nowMs);
  gLastEffectiveReadings = app::buildEffectiveReadings(gLastRealReadings, gDemoOverrides, nowMs);
  const app::LogicResult& logicResult = gStateMachine.update(gLastEffectiveReadings, nowMs);

  gCloud.update(nowMs, gLastEffectiveReadings, logicResult);
  applyOutputs(logicResult);

  if (logicResult.stateChangedThisCycle) {
    logStateTransition(logicResult);
    gLastStateLogMs = nowMs;
  }

  bool loggedSensorThisCycle = false;
  if (gLastRealReadings.dhtReadAttemptedThisCycle && gLastRealReadings.dhtLastReadFailed &&
      !gLastEffectiveReadings.tempOverrideActive) {
    logSensorReadings(gLastRealReadings, gLastEffectiveReadings, logicResult);
    gLastSensorLogMs = nowMs;
    loggedSensorThisCycle = true;
  }

  if (!loggedSensorThisCycle &&
      (gLastSensorLogMs == 0 || nowMs - gLastSensorLogMs >= app::kSensorLogIntervalMs)) {
    gLastSensorLogMs = nowMs;
    logSensorReadings(gLastRealReadings, gLastEffectiveReadings, logicResult);
  }

  if (gLastStateLogMs == 0 || nowMs - gLastStateLogMs >= app::kStateLogIntervalMs) {
    gLastStateLogMs = nowMs;
    logStateSnapshot(logicResult, nowMs);
  }
}
1