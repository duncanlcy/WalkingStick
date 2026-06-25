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
constexpr uint16_t MIN_PRESSURE_THRESHOLD = 100;
constexpr uint8_t LOW_BATTERY_PERCENT = 15;

// Component health and model fail-safe
constexpr uint8_t MAX_SENSOR_FAILURES = 5;
constexpr float ACCEL_MIN_VALID_G = 0.1f;
constexpr float ACCEL_MAX_VALID_G = 16.0f;
constexpr float ACCEL_STUCK_TOLERANCE_G = 0.01f;
constexpr uint16_t ADC_SATURATED_THRESHOLD = 4090;
constexpr uint16_t ADC_STUCK_TOLERANCE = 2;
constexpr uint32_t MODEL_RECOVERY_REBOOT_MS = 10000;
constexpr bool MODEL_AUTO_RECOVERY_REBOOT = true;
}  // namespace config
