#pragma once

#include "config.h"
#include "model_metrics.h"
#include "model_rollout.h"
#include "protocol.h"
#include "safety.h"
#include "sensors.h"

// Predictive safety layer that applies rollout-stage sensitivity and records outcomes.
class GaitPredictor {
 public:
  GaitPredictor()
      : safety_(config::FALL_ACCEL_THRESHOLD_G, config::IMPACT_THRESHOLD_G) {}

  void setRolloutConfig(const RolloutConfig& cfg) {
    rollout_.setConfig(cfg);
    applyThresholds();
  }

  RolloutManager& rollout() { return rollout_; }
  ModelMetrics& metrics() { return metrics_; }

  PredictionResult evaluateGait(const PressureReading& pressure, DeviceRole source) {
    applyThresholds();

    const float imbalance = computeImbalance(pressure);
    PredictionResult result{};
    result.timestamp_ms = millis();
    result.source = source;
    result.confidence = imbalance;
    result.outcome = OUTCOME_NONE;

    metrics_.recordPrediction();

    if (imbalance < rollout_.effectiveGaitThreshold()) {
      metrics_.recordOutcome(OUTCOME_TRUE_NEGATIVE);
      return result;
    }

    result.alert = safety_.evaluateGait(pressure, source, rollout_.effectiveGaitThreshold());

    if (rollout_.elevateWarnings() && result.alert.level == ALERT_WARNING) {
      result.alert.level = ALERT_CRITICAL;
      snprintf(result.alert.message, sizeof(result.alert.message),
               "High-sensitivity warning: %s", result.alert.message);
    }

    result.is_abnormal = result.alert.level != ALERT_NONE;
    pending_gait_warning_ = result.is_abnormal;
    warning_timestamp_ms_ = result.timestamp_ms;

    return result;
  }

  PredictionResult evaluateAccel(const AccelerometerReading& accel, DeviceRole source) {
    applyThresholds();

    PredictionResult result{};
    result.timestamp_ms = millis();
    result.source = source;
    result.confidence = accel.magnitude();
    result.outcome = OUTCOME_NONE;

    metrics_.recordPrediction();
    result.alert = safety_.evaluate(accel, source);

    if (result.alert.level == ALERT_NONE) {
      metrics_.recordOutcome(OUTCOME_TRUE_NEGATIVE);
      return result;
    }

    result.is_abnormal = true;
    return result;
  }

  // Call after a warning when gait returns to normal without impact — records a true positive.
  bool checkFallPrevention(const PressureReading& pressure) {
    if (!pending_gait_warning_) {
      return false;
    }

    const float imbalance = computeImbalance(pressure);
    const uint32_t elapsed = millis() - warning_timestamp_ms_;

    if (elapsed > config::PREVENTION_WINDOW_MS) {
      pending_gait_warning_ = false;
      metrics_.recordOutcome(OUTCOME_FALSE_POSITIVE);
      return false;
    }

    if (imbalance < rollout_.effectiveGaitThreshold()) {
      pending_gait_warning_ = false;
      metrics_.recordFallPrevention();
      return true;
    }

    return false;
  }

 private:
  static float computeImbalance(const PressureReading& pressure) {
    const uint16_t left = pressure.leftTotal();
    const uint16_t right = pressure.rightTotal();
    const uint16_t total = left + right;

    if (total < config::MIN_PRESSURE_THRESHOLD) {
      return 0.0f;
    }

    return fabsf(static_cast<float>(left) - static_cast<float>(right)) /
           static_cast<float>(total);
  }

  void applyThresholds() {
    const float fall_mult = rollout_.fallThresholdMultiplier();
    safety_ = SafetyMonitor(
        config::FALL_ACCEL_THRESHOLD_G * fall_mult,
        config::IMPACT_THRESHOLD_G * fall_mult);
  }

  SafetyMonitor safety_;
  RolloutManager rollout_;
  ModelMetrics metrics_;
  bool pending_gait_warning_ = false;
  uint32_t warning_timestamp_ms_ = 0;
};
