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

// Stick location tracing (BLE RSSI trilateration against fixed room anchors)
constexpr uint32_t POSITION_SAMPLE_MS = 1000;
constexpr size_t POSITION_HISTORY_CAPACITY = 32;
constexpr int8_t RSSI_AT_1M_DBM = -59;
constexpr float PATH_LOSS_EXPONENT = 2.0f;
constexpr size_t MAX_BEACON_ANCHORS = 4;

// Safe zone for cognitive / Alzheimer's wandering detection (meters from home anchor)
constexpr float SAFE_ZONE_MIN_X_M = -5.0f;
constexpr float SAFE_ZONE_MAX_X_M = 5.0f;
constexpr float SAFE_ZONE_MIN_Y_M = -5.0f;
constexpr float SAFE_ZONE_MAX_Y_M = 5.0f;
constexpr float WANDERING_DISPLACEMENT_M = 8.0f;
constexpr uint32_t WANDERING_WINDOW_MS = 300000;  // 5 minutes
constexpr float ERRATIC_SPEED_CV_THRESHOLD = 1.2f;
constexpr float STICK_TILT_WARNING_DEG = 35.0f;
constexpr float STICK_TILT_CRITICAL_DEG = 55.0f;
}  // namespace config
