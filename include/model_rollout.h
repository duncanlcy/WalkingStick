#pragma once

#include <Arduino.h>
#include "config.h"

// Staged deployment phases for the predictive gait model.
enum RolloutStage : uint8_t {
  ROLLOUT_INITIAL = 0,       // Stage 1: high warning sensitivity for abnormal walking
  ROLLOUT_CALIBRATING = 1,   // Gradual transition toward client threshold
  ROLLOUT_CLIENT_TUNED = 2,  // Client controls caution level via safety threshold
};

// Client-configurable caution level (0 = least cautious, 100 = most cautious).
struct RolloutConfig {
  RolloutStage stage = ROLLOUT_INITIAL;
  uint8_t client_safety_threshold = config::DEFAULT_CLIENT_SAFETY_THRESHOLD;
  uint8_t calibration_progress = 0;  // 0-100, used during ROLLOUT_CALIBRATING
};

class RolloutManager {
 public:
  explicit RolloutManager(RolloutConfig cfg = {}) : config_(cfg) {}

  const RolloutConfig& config() const { return config_; }

  void setConfig(const RolloutConfig& cfg) { config_ = cfg; }

  void setStage(RolloutStage stage) { config_.stage = stage; }

  void setClientSafetyThreshold(uint8_t threshold) {
    config_.client_safety_threshold = constrain(threshold, 0, 100);
  }

  // Effective gait imbalance threshold after rollout stage and client tuning.
  float effectiveGaitThreshold() const {
    const float base = config::GAIT_IMBALANCE_THRESHOLD;
    const float client = config::clientGaitThreshold(config_.client_safety_threshold);

    switch (config_.stage) {
      case ROLLOUT_INITIAL:
        return config::INITIAL_ROLLOUT_GAIT_THRESHOLD;
      case ROLLOUT_CALIBRATING: {
        const float t = config_.calibration_progress / 100.0f;
        const float initial = config::INITIAL_ROLLOUT_GAIT_THRESHOLD;
        return initial + t * (client - initial);
      }
      case ROLLOUT_CLIENT_TUNED:
      default:
        return client;
    }
  }

  // Sensitivity multiplier applied to fall/impact thresholds (lower = more sensitive).
  float fallThresholdMultiplier() const {
    switch (config_.stage) {
      case ROLLOUT_INITIAL:
        return config::INITIAL_ROLLOUT_FALL_MULTIPLIER;
      case ROLLOUT_CALIBRATING: {
        const float t = config_.calibration_progress / 100.0f;
        const float initial = config::INITIAL_ROLLOUT_FALL_MULTIPLIER;
        const float client = config::clientFallMultiplier(config_.client_safety_threshold);
        return initial + t * (client - initial);
      }
      case ROLLOUT_CLIENT_TUNED:
      default:
        return config::clientFallMultiplier(config_.client_safety_threshold);
    }
  }

  // During initial rollout, abnormal walking warnings are elevated.
  bool elevateWarnings() const {
    return config_.stage == ROLLOUT_INITIAL;
  }

 private:
  RolloutConfig config_;
};
