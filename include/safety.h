#pragma once

#include "config.h"
#include "protocol.h"
#include "sensors.h"

class SafetyMonitor {
 public:
  explicit SafetyMonitor(float fall_threshold_g = config::FALL_ACCEL_THRESHOLD_G,
                         float impact_threshold_g = config::IMPACT_THRESHOLD_G)
      : fall_threshold_g_(fall_threshold_g),
        impact_threshold_g_(impact_threshold_g) {}

  AlertEvent evaluate(const AccelerometerReading& accel, DeviceRole source) {
    AlertEvent event{};
    event.timestamp_ms = millis();
    event.source = source;
    event.level = ALERT_NONE;
    event.type = ALERT_TYPE_NONE;
    event.message[0] = '\0';

    const float magnitude = accel.magnitude();

    if (magnitude >= impact_threshold_g_) {
      event.level = ALERT_CRITICAL;
      event.type = ALERT_TYPE_IMPACT;
      snprintf(event.message, sizeof(event.message), "Impact detected: %.2fg", magnitude);
      return event;
    }

    if (magnitude >= fall_threshold_g_) {
      event.level = ALERT_WARNING;
      event.type = ALERT_TYPE_FALL_DETECTED;
      snprintf(event.message, sizeof(event.message), "Possible fall: %.2fg", magnitude);
      return event;
    }

    return event;
  }

  AlertEvent evaluateGait(const PressureReading& pressure, DeviceRole source) {
    AlertEvent event{};
    event.timestamp_ms = millis();
    event.source = source;
    event.level = ALERT_NONE;
    event.type = ALERT_TYPE_NONE;
    event.message[0] = '\0';

    const uint16_t left = pressure.leftTotal();
    const uint16_t right = pressure.rightTotal();
    const uint16_t total = left + right;

    if (total < config::MIN_PRESSURE_THRESHOLD) {
      return event;
    }

    const float imbalance = fabsf(static_cast<float>(left) - static_cast<float>(right)) /
                            static_cast<float>(total);

    if (imbalance > 0.6f) {
      event.level = ALERT_WARNING;
      event.type = ALERT_TYPE_GAIT_IRREGULAR;
      snprintf(event.message, sizeof(event.message), "Gait imbalance: %.0f%%", imbalance * 100.0f);
    }

    return event;
  }

 private:
  float fall_threshold_g_;
  float impact_threshold_g_;
};
