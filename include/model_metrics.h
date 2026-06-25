#pragma once

#include <Arduino.h>
#include "protocol.h"

// Tracks model prediction outcomes for operational review and retraining.
class ModelMetrics {
 public:
  uint32_t true_positives() const { return true_positives_; }
  uint32_t false_positives() const { return false_positives_; }
  uint32_t true_negatives() const { return true_negatives_; }
  uint32_t false_negatives() const { return false_negatives_; }
  uint32_t fall_preventions() const { return fall_preventions_; }
  uint32_t total_predictions() const { return total_predictions_; }

  void recordPrediction() { total_predictions_++; }

  void recordOutcome(PredictionOutcome outcome) {
    switch (outcome) {
      case OUTCOME_TRUE_POSITIVE:
        true_positives_++;
        fall_preventions_++;
        break;
      case OUTCOME_FALSE_POSITIVE:
        false_positives_++;
        break;
      case OUTCOME_TRUE_NEGATIVE:
        true_negatives_++;
        break;
      case OUTCOME_FALSE_NEGATIVE:
        false_negatives_++;
        break;
      case OUTCOME_NONE:
      default:
        break;
    }
  }

  // A successful fall prevention: warning issued and gait normalized without impact.
  void recordFallPrevention() {
    recordOutcome(OUTCOME_TRUE_POSITIVE);
  }

  ModelMetricsSnapshot snapshot() const {
    ModelMetricsSnapshot snap{};
    snap.true_positives = true_positives_;
    snap.false_positives = false_positives_;
    snap.true_negatives = true_negatives_;
    snap.false_negatives = false_negatives_;
    snap.fall_preventions = fall_preventions_;
    snap.total_predictions = total_predictions_;
    return snap;
  }

  void reset() {
    true_positives_ = 0;
    false_positives_ = 0;
    true_negatives_ = 0;
    false_negatives_ = 0;
    fall_preventions_ = 0;
    total_predictions_ = 0;
  }

 private:
  uint32_t true_positives_ = 0;
  uint32_t false_positives_ = 0;
  uint32_t true_negatives_ = 0;
  uint32_t false_negatives_ = 0;
  uint32_t fall_preventions_ = 0;
  uint32_t total_predictions_ = 0;
};
