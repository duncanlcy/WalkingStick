#pragma once

#include <Arduino.h>
#include "protocol.h"

// Lightweight in-memory ring buffer for telemetry when SD is unavailable.
// Replace with SD/SPIFFS writer on waist safety pad hardware.
class DataLogger {
 public:
  static constexpr size_t kCapacity = 64;

  bool log(const TelemetryPacket& packet) {
    buffer_[head_] = packet;
    head_ = (head_ + 1) % kCapacity;
    if (count_ < kCapacity) {
      count_++;
    }
    return true;
  }

  bool logOutcome(PredictionOutcome outcome, const SensorSample& sample) {
    TelemetryPacket packet{};
    packet.protocol_version = PROTOCOL_VERSION;
    packet.source = sample.source;
    packet.sample = sample;
    packet.has_alert = outcome == OUTCOME_TRUE_POSITIVE || outcome == OUTCOME_FALSE_POSITIVE;
    packet.has_metrics = true;
    if (packet.has_alert) {
      packet.alert.timestamp_ms = sample.timestamp_ms;
      packet.alert.source = sample.source;
      packet.alert.level = outcome == OUTCOME_TRUE_POSITIVE ? ALERT_WARNING : ALERT_INFO;
      packet.alert.type = ALERT_TYPE_GAIT_PREDICTION;
      snprintf(packet.alert.message, sizeof(packet.alert.message),
               outcome == OUTCOME_TRUE_POSITIVE ? "Fall prevention recorded" : "Prediction outcome logged");
    }
    return log(packet);
  }

  size_t count() const { return count_; }

  bool get(size_t index, TelemetryPacket& out) const {
    if (index >= count_) {
      return false;
    }
    const size_t start = (head_ + kCapacity - count_) % kCapacity;
    out = buffer_[(start + index) % kCapacity];
    return true;
  }

  void clear() {
    head_ = 0;
    count_ = 0;
  }

 private:
  TelemetryPacket buffer_[kCapacity]{};
  size_t head_ = 0;
  size_t count_ = 0;
};
