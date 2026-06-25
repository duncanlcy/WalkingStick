#pragma once

#include <Arduino.h>
#include "device_roles.h"

// BLE service UUIDs — shared across all nodes
#define WALKING_STICK_SERVICE_UUID "a1b2c3d4-e5f6-7890-abcd-ef1234567890"
#define TELEMETRY_CHAR_UUID "a1b2c3d4-e5f6-7890-abcd-ef1234567891"
#define COMMAND_CHAR_UUID "a1b2c3d4-e5f6-7890-abcd-ef1234567892"
#define ALERT_CHAR_UUID "a1b2c3d4-e5f6-7890-abcd-ef1234567893"

enum AlertLevel : uint8_t {
  ALERT_NONE = 0,
  ALERT_INFO = 1,
  ALERT_WARNING = 2,
  ALERT_CRITICAL = 3,
};

enum AlertType : uint8_t {
  ALERT_TYPE_NONE = 0,
  ALERT_TYPE_FALL_DETECTED = 1,
  ALERT_TYPE_IMPACT = 2,
  ALERT_TYPE_GAIT_IRREGULAR = 3,
  ALERT_TYPE_LOW_BATTERY = 4,
  ALERT_TYPE_SOS = 5,
  ALERT_TYPE_GAIT_PREDICTION = 6,
};

enum PredictionOutcome : uint8_t {
  OUTCOME_NONE = 0,
  OUTCOME_TRUE_POSITIVE = 1,   // Warning issued, fall successfully prevented
  OUTCOME_FALSE_POSITIVE = 2,  // Warning issued, no actual risk materialized
  OUTCOME_TRUE_NEGATIVE = 3,   // No warning, no fall
  OUTCOME_FALSE_NEGATIVE = 4,  // No warning, fall occurred
};

struct SensorSample {
  uint32_t timestamp_ms;
  float accel_x;
  float accel_y;
  float accel_z;
  uint16_t pressure_left;
  uint16_t pressure_right;
  uint8_t battery_percent;
  DeviceRole source;
};

struct AlertEvent {
  uint32_t timestamp_ms;
  AlertLevel level;
  AlertType type;
  DeviceRole source;
  char message[64];
};

struct ModelMetricsSnapshot {
  uint32_t true_positives;
  uint32_t false_positives;
  uint32_t true_negatives;
  uint32_t false_negatives;
  uint32_t fall_preventions;
  uint32_t total_predictions;
};

struct PredictionResult {
  uint32_t timestamp_ms;
  DeviceRole source;
  AlertEvent alert{};
  float confidence;
  PredictionOutcome outcome;
  bool is_abnormal;
};

struct RolloutConfigPacket {
  uint8_t stage;
  uint8_t client_safety_threshold;
  uint8_t calibration_progress;
};

struct TelemetryPacket {
  uint8_t protocol_version;
  DeviceRole source;
  SensorSample sample;
  AlertEvent alert;
  bool has_alert;
  bool has_metrics;
  ModelMetricsSnapshot metrics;
};
