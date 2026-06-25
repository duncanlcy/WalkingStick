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
  ALERT_TYPE_SENSOR_FAULT = 6,
  ALERT_TYPE_MODEL_STOPPED = 7,
};

enum ModelState : uint8_t {
  MODEL_STATE_RUNNING = 0,
  MODEL_STATE_STOPPED = 1,
  MODEL_STATE_RECOVERY = 2,
};

enum CommandType : uint8_t {
  CMD_NONE = 0,
  CMD_START_MODEL = 1,
  CMD_STOP_MODEL = 2,
  CMD_REBOOT_DEVICE = 3,
};

struct CommandPacket {
  CommandType type = CMD_NONE;
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

struct TelemetryPacket {
  uint8_t protocol_version;
  DeviceRole source;
  SensorSample sample;
  AlertEvent alert;
  bool has_alert;
  ModelState model_state;
  bool sensor_healthy;
};
