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
constexpr int BUTTON_PLAY = 12;
constexpr int BUTTON_NEXT = 13;
constexpr int BUTTON_VOLUME = 14;
constexpr int BUTTON_RECOMMEND = 15;
constexpr int VIBRATOR = 26;
constexpr int STATUS_LED = 2;
constexpr int BATTERY_ADC = 36;
constexpr int I2S_BCK = 27;
constexpr int I2S_LRCK = 32;
constexpr int I2S_DOUT = 33;
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

// Elderly-friendly media input timing
constexpr uint32_t BUTTON_DEBOUNCE_MS = 80;
constexpr uint32_t BUTTON_LONG_PRESS_MS = 800;
constexpr uint8_t MEDIA_DEFAULT_VOLUME = 40;
constexpr uint8_t MEDIA_MAX_VOLUME = 70;
constexpr uint8_t MEDIA_VOLUME_STEP = 10;
constexpr uint8_t MEDIA_MAX_RECOMMENDATIONS = 5;
}  // namespace config
