#pragma once

#include <Arduino.h>
#include <math.h>

#include "config.h"
#include "sensors.h"

enum ComponentStatus : uint8_t {
  COMPONENT_OK = 0,
  COMPONENT_DEGRADED = 1,
  COMPONENT_FAILED = 2,
};

struct SensorHealth {
  ComponentStatus status = COMPONENT_OK;
  uint8_t consecutive_failures = 0;
  char reason[32]{};
};

class ComponentHealthMonitor {
 public:
  SensorHealth checkAccelerometer(const AccelerometerReading& reading) {
    SensorHealth health{};

    if (!reading.hardware_ok) {
      return recordAccelFailure("accel hardware error");
    }

    if (!isfinite(reading.x) || !isfinite(reading.y) || !isfinite(reading.z)) {
      return recordAccelFailure("accel non-finite");
    }

    const float magnitude = reading.magnitude();
    if (!isfinite(magnitude)) {
      return recordAccelFailure("accel magnitude invalid");
    }

    if (magnitude < config::ACCEL_MIN_VALID_G || magnitude > config::ACCEL_MAX_VALID_G) {
      return recordAccelFailure("accel out of range");
    }

    if (has_previous_accel_) {
      const float delta = fabsf(magnitude - previous_accel_magnitude_);
      if (delta < config::ACCEL_STUCK_TOLERANCE_G &&
          consecutive_accel_failures_ >= config::MAX_SENSOR_FAILURES / 2) {
        return recordAccelFailure("accel reading stuck");
      }
    }

    previous_accel_magnitude_ = magnitude;
    has_previous_accel_ = true;
    consecutive_accel_failures_ = 0;
    return health;
  }

  SensorHealth checkPressure(const PressureReading& reading) {
    SensorHealth health{};

    if (!reading.hardware_ok) {
      return recordPressureFailure("pressure hardware error");
    }

    const uint16_t channels[4] = {
        reading.left_heel, reading.left_toe, reading.right_heel, reading.right_toe};

    for (uint16_t value : channels) {
      if (value >= config::ADC_SATURATED_THRESHOLD) {
        return recordPressureFailure("pressure adc saturated");
      }
    }

    if (has_previous_pressure_) {
      bool all_stuck = true;
      for (size_t i = 0; i < 4; ++i) {
        const int delta = abs(static_cast<int>(channels[i]) - static_cast<int>(previous_pressure_[i]));
        if (delta > config::ADC_STUCK_TOLERANCE) {
          all_stuck = false;
          break;
        }
      }

      if (all_stuck && reading.total() > 0 &&
          consecutive_pressure_failures_ >= config::MAX_SENSOR_FAILURES / 2) {
        return recordPressureFailure("pressure reading stuck");
      }
    }

    memcpy(previous_pressure_, channels, sizeof(previous_pressure_));
    has_previous_pressure_ = true;
    consecutive_pressure_failures_ = 0;
    return health;
  }

  bool accelerometerHealthy() const {
    return consecutive_accel_failures_ < config::MAX_SENSOR_FAILURES;
  }

  bool pressureHealthy() const {
    return consecutive_pressure_failures_ < config::MAX_SENSOR_FAILURES;
  }

 private:
  SensorHealth recordAccelFailure(const char* reason) {
    SensorHealth health{};
    health.status = COMPONENT_FAILED;
    strncpy(health.reason, reason, sizeof(health.reason) - 1);
    health.reason[sizeof(health.reason) - 1] = '\0';
    ++consecutive_accel_failures_;
    health.consecutive_failures = consecutive_accel_failures_;
    return health;
  }

  SensorHealth recordPressureFailure(const char* reason) {
    SensorHealth health{};
    health.status = COMPONENT_FAILED;
    strncpy(health.reason, reason, sizeof(health.reason) - 1);
    health.reason[sizeof(health.reason) - 1] = '\0';
    ++consecutive_pressure_failures_;
    health.consecutive_failures = consecutive_pressure_failures_;
    return health;
  }

  float previous_accel_magnitude_ = 0.0f;
  bool has_previous_accel_ = false;
  uint16_t previous_pressure_[4]{};
  bool has_previous_pressure_ = false;
  uint8_t consecutive_accel_failures_ = 0;
  uint8_t consecutive_pressure_failures_ = 0;
};
