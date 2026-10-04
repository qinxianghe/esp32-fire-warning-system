#include "actuators.h"

#include "pins.h"

namespace app {

void ActuatorManager::begin() {
  pinMode(pins::kGreenLedOutput, OUTPUT);
  pinMode(pins::kYellowLedOutput, OUTPUT);
  pinMode(pins::kRedLedOutput, OUTPUT);
  pinMode(pins::kBuzzerOutput, OUTPUT);
  pinMode(pins::kRelayOutput, OUTPUT);
  allOff();
}

void ActuatorManager::allOff() {
  setGreenLed(false);
  setYellowLed(false);
  setRedLed(false);
  setBuzzer(false);
  setRelay(false);
}

void ActuatorManager::setGreenLed(bool enabled) {
  digitalWrite(pins::kGreenLedOutput, enabled ? HIGH : LOW);
}

void ActuatorManager::setYellowLed(bool enabled) {
  digitalWrite(pins::kYellowLedOutput, enabled ? HIGH : LOW);
}

void ActuatorManager::setRedLed(bool enabled) {
  digitalWrite(pins::kRedLedOutput, enabled ? HIGH : LOW);
}

void ActuatorManager::setBuzzer(bool enabled) {
  digitalWrite(pins::kBuzzerOutput, enabled ? HIGH : LOW);
}

void ActuatorManager::setRelay(bool enabled) {
  digitalWrite(pins::kRelayOutput, enabled ? HIGH : LOW);
}

}  // namespace app
