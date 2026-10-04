#pragma once

#include <Arduino.h>

// Shared helper functions and formatting utilities belong here.

namespace app {

inline void logBanner(const char* zoneId) {
  Serial.println();
  Serial.println("[boot] Multi-zone library fire warning node");
  Serial.print("[boot] Zone: ");
  Serial.println(zoneId);
}

}  // namespace app
