#pragma once

// Per-zone GPIO assignments for the current ESP32 Dev Module placeholder wiring.
// Keep this file aligned with docs/pinmap.md whenever the hardware map changes.

namespace pins {

constexpr int kMq2AnalogInput = 34;    // MQ-2 analog signal input.
constexpr int kDht11Data = 4;           // DHT11 single-wire data line.
constexpr int kFlameDigitalInput = 27;  // IR flame sensor digital output.
constexpr int kGreenLedOutput = 16;     // Green LED for normal state.
constexpr int kYellowLedOutput = 17;    // Yellow LED for warning state.
constexpr int kRedLedOutput = 19;       // Red LED for alarm state.
constexpr int kBuzzerOutput = 18;       // Buzzer output for audible alarm.
constexpr int kRelayOutput = 23;        // Relay output for linkage demo.

// Backward-compatible aliases for the current scaffold.
constexpr int kMq2 = kMq2AnalogInput;
constexpr int kDht11 = kDht11Data;
constexpr int kFlame = kFlameDigitalInput;
constexpr int kGreenLed = kGreenLedOutput;
constexpr int kYellowLed = kYellowLedOutput;
constexpr int kRedLed = kRedLedOutput;
constexpr int kBuzzer = kBuzzerOutput;
constexpr int kRelay = kRelayOutput;

}  // namespace pins
