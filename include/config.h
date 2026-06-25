#pragma once

#ifndef BLE_DEVICE_NAME
#define BLE_DEVICE_NAME "WalkingStick"
#endif

// Pin assignments — adjust per PCB revision (see docs/hardware.md)
namespace pins {

namespace waist {
constexpr int MPU_INT = 4;
constexpr int BUZZER = 25;
constexpr int STATUS_LED = 2;
constexpr int SD_CS = 5;
}  // namespace waist

namespace shoe {
constexpr int FSR_LEFT_HEEL = 34;
constexpr int FSR_LEFT_TOE = 35;
constexpr int FSR_RIGHT_HEEL = 32;
constexpr int FSR_RIGHT_TOE = 33;
constexpr int STATUS_LED = 2;
}  // namespace shoe

namespace stick {
constexpr int BUTTON_ALERT = 0;
constexpr int VIBRATOR = 26;
constexpr int STATUS_LED = 2;
constexpr int BATTERY_ADC = 36;
}  // namespace stick

}  // namespace pins

// Sampling and safety thresholds
namespace config {
constexpr uint32_t SENSOR_SAMPLE_MS = 50;
constexpr uint32_t TELEMETRY_INTERVAL_MS = 200;
constexpr uint32_t BLE_SCAN_INTERVAL_MS = 5000;

constexpr float FALL_ACCEL_THRESHOLD_G = 2.5f;
constexpr float IMPACT_THRESHOLD_G = 4.0f;
constexpr float GAIT_IMBALANCE_THRESHOLD = 0.6f;
constexpr uint16_t MIN_PRESSURE_THRESHOLD = 100;
constexpr uint8_t LOW_BATTERY_PERCENT = 15;

// Staged rollout defaults — stage 1 uses high warning sensitivity
constexpr float INITIAL_ROLLOUT_GAIT_THRESHOLD = 0.35f;
constexpr float INITIAL_ROLLOUT_FALL_MULTIPLIER = 0.75f;
constexpr uint8_t DEFAULT_CLIENT_SAFETY_THRESHOLD = 70;
constexpr uint32_t PREVENTION_WINDOW_MS = 10000;

// Map client safety threshold (0-100) to detection parameters.
inline float clientGaitThreshold(uint8_t safety_threshold) {
  const float t = safety_threshold / 100.0f;
  return GAIT_IMBALANCE_THRESHOLD - t * 0.25f;
}

inline float clientFallMultiplier(uint8_t safety_threshold) {
  const float t = safety_threshold / 100.0f;
  return 1.0f - t * 0.3f;
}
}  // namespace config
