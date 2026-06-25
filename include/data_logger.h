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
