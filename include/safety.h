#pragma once

#include "config.h"
#include "protocol.h"
#include "sensors.h"
#include "location.h"

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

  AlertEvent evaluatePosition(const PositionSample& position, DeviceRole source) {
    AlertEvent event = makeEmptyAlert(source);

    if (!isInsideSafeZone(position)) {
      event.level = ALERT_WARNING;
      event.type = ALERT_TYPE_POSITION_IRREGULAR;
      snprintf(event.message, sizeof(event.message),
               "Outside safe zone (%.1f, %.1f m)", position.x_m, position.y_m);
    }

    return event;
  }

  AlertEvent evaluateCognitiveMovement(const StickLocationTracker& tracker, DeviceRole source) {
    AlertEvent event = makeEmptyAlert(source);

    if (!tracker.hasFix()) {
      return event;
    }

    const float displacement = tracker.displacementSince(config::WANDERING_WINDOW_MS);
    if (displacement >= config::WANDERING_DISPLACEMENT_M) {
      event.level = ALERT_WARNING;
      event.type = ALERT_TYPE_COGNITIVE_WANDERING;
      snprintf(event.message, sizeof(event.message),
               "Wandering detected: %.1f m in 5 min", displacement);
      return event;
    }

    const float speed_cv = tracker.speedCoefficientOfVariation(config::WANDERING_WINDOW_MS);
    if (speed_cv >= config::ERRATIC_SPEED_CV_THRESHOLD) {
      event.level = ALERT_WARNING;
      event.type = ALERT_TYPE_POSITION_IRREGULAR;
      snprintf(event.message, sizeof(event.message),
               "Erratic movement (cognitive risk)");
      return event;
    }

    const PositionSample& position = tracker.latest();
    if (!isInsideSafeZone(position)) {
      event.level = ALERT_CRITICAL;
      event.type = ALERT_TYPE_COGNITIVE_WANDERING;
      snprintf(event.message, sizeof(event.message),
               "Patient left safe area — check immediately");
    }

    return event;
  }

  AlertEvent evaluateStickPosture(const AccelerometerReading& accel, DeviceRole source) {
    AlertEvent event = makeEmptyAlert(source);

    const float tilt_deg = stickTiltDegrees(accel);
    if (tilt_deg >= config::STICK_TILT_CRITICAL_DEG) {
      event.level = ALERT_CRITICAL;
      event.type = ALERT_TYPE_STICK_POSTURE_ABNORMAL;
      snprintf(event.message, sizeof(event.message),
               "Stick dropped or unsafe tilt: %.0f deg", tilt_deg);
      return event;
    }

    if (tilt_deg >= config::STICK_TILT_WARNING_DEG) {
      event.level = ALERT_WARNING;
      event.type = ALERT_TYPE_STICK_POSTURE_ABNORMAL;
      snprintf(event.message, sizeof(event.message),
               "Irregular stick posture: %.0f deg", tilt_deg);
    }

    return event;
  }

 private:
  static AlertEvent makeEmptyAlert(DeviceRole source) {
    AlertEvent event{};
    event.timestamp_ms = millis();
    event.source = source;
    event.level = ALERT_NONE;
    event.type = ALERT_TYPE_NONE;
    event.message[0] = '\0';
    return event;
  }

  static float stickTiltDegrees(const AccelerometerReading& accel) {
    const float horizontal = sqrtf(accel.x * accel.x + accel.y * accel.y);
    return atan2f(horizontal, fabsf(accel.z)) * 180.0f / PI;
  }

  float fall_threshold_g_;
  float impact_threshold_g_;
};
