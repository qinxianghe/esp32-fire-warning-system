#pragma once

#include <Arduino.h>

// LED, buzzer, and relay output control is declared here.
// This module only owns output pin setup and output state changes.

namespace app {

class ActuatorManager {
 public:
  void begin();
  void allOff();
  void setGreenLed(bool enabled);
  void setYellowLed(bool enabled);
  void setRedLed(bool enabled);
  void setBuzzer(bool enabled);
  void setRelay(bool enabled);
};

}  // namespace app
