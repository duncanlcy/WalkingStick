#pragma once

#include <Arduino.h>
#include <math.h>

#include "config.h"
#include "device_roles.h"
#include "protocol.h"

struct BeaconReading {
  DeviceRole role;
  int8_t rssi_dbm;
};

struct AnchorBeacon {
  DeviceRole role;
  float x_m;
  float y_m;
};

// Estimates stick position from BLE RSSI and maintains a trace history.
class StickLocationTracker {
 public:
  StickLocationTracker() { resetAnchorsToDefaults(); }

  void resetAnchorsToDefaults() {
    anchor_count_ = 3;
    anchors_[0] = {DEVICE_WAIST_SAFETY_PAD, 0.0f, 0.0f};
    anchors_[1] = {DEVICE_SHOE_PAD, 0.5f, -0.3f};
    anchors_[2] = {DEVICE_WALKING_STICK, 0.0f, 0.8f};
  }

  void setAnchor(DeviceRole role, float x_m, float y_m) {
    for (size_t i = 0; i < anchor_count_; ++i) {
      if (anchors_[i].role == role) {
        anchors_[i].x_m = x_m;
        anchors_[i].y_m = y_m;
        return;
      }
    }
    if (anchor_count_ < config::MAX_BEACON_ANCHORS) {
      anchors_[anchor_count_++] = {role, x_m, y_m};
    }
  }

  float rssiToDistanceM(int8_t rssi_dbm) const {
    const float ratio = static_cast<float>(rssi_dbm) / static_cast<float>(config::RSSI_AT_1M_DBM);
    return powf(10.0f, (1.0f - ratio) / (10.0f * config::PATH_LOSS_EXPONENT));
  }

  PositionSample estimatePosition(const BeaconReading* readings, size_t count) {
    PositionSample sample{};
    sample.timestamp_ms = millis();
    sample.source = DEVICE_WALKING_STICK;
    sample.accuracy_m = 99.0f;

    if (count == 0) {
      return sample;
    }

    float sum_weight = 0.0f;
    float est_x = 0.0f;
    float est_y = 0.0f;
    size_t matched = 0;

    for (size_t i = 0; i < count; ++i) {
      const AnchorBeacon* anchor = findAnchor(readings[i].role);
      if (!anchor) {
        continue;
      }

      const float distance = rssiToDistanceM(readings[i].rssi_dbm);
      const float weight = 1.0f / fmaxf(distance * distance, 0.01f);
      est_x += anchor->x_m * weight;
      est_y += anchor->y_m * weight;
      sum_weight += weight;
      matched++;
    }

    if (matched == 0 || sum_weight <= 0.0f) {
      return sample;
    }

    sample.x_m = est_x / sum_weight;
    sample.y_m = est_y / sum_weight;
    sample.accuracy_m = 1.0f / sqrtf(sum_weight);
    recordSample(sample);
    return sample;
  }

  const PositionSample& latest() const { return latest_; }
  bool hasFix() const { return has_fix_; }

  size_t historyCount() const { return history_count_; }

  bool getHistory(size_t index, PositionSample& out) const {
    if (index >= history_count_) {
      return false;
    }
    const size_t start = (history_head_ + config::POSITION_HISTORY_CAPACITY - history_count_) %
                         config::POSITION_HISTORY_CAPACITY;
    out = history_[(start + index) % config::POSITION_HISTORY_CAPACITY];
    return true;
  }

  void clearHistory() {
    history_head_ = 0;
    history_count_ = 0;
    has_fix_ = false;
  }

  float displacementSince(uint32_t window_ms) const {
    if (history_count_ < 2) {
      return 0.0f;
    }

    PositionSample oldest{};
    PositionSample newest = latest_;
    const uint32_t cutoff = newest.timestamp_ms - window_ms;

    for (size_t i = 0; i < history_count_; ++i) {
      PositionSample sample{};
      if (!getHistory(i, sample)) {
        continue;
      }
      if (sample.timestamp_ms >= cutoff) {
        oldest = sample;
        break;
      }
    }

    const float dx = newest.x_m - oldest.x_m;
    const float dy = newest.y_m - oldest.y_m;
    return sqrtf(dx * dx + dy * dy);
  }

  float speedCoefficientOfVariation(uint32_t window_ms) const {
    if (history_count_ < 3) {
      return 0.0f;
    }

    float speeds[config::POSITION_HISTORY_CAPACITY]{};
    size_t speed_count = 0;

    for (size_t i = 1; i < history_count_; ++i) {
      PositionSample prev{};
      PositionSample curr{};
      if (!getHistory(i - 1, prev) || !getHistory(i, curr)) {
        continue;
      }
      if (curr.timestamp_ms < millis() - window_ms) {
        continue;
      }

      const uint32_t dt_ms = curr.timestamp_ms - prev.timestamp_ms;
      if (dt_ms == 0) {
        continue;
      }

      const float dx = curr.x_m - prev.x_m;
      const float dy = curr.y_m - prev.y_m;
      const float speed = sqrtf(dx * dx + dy * dy) / (static_cast<float>(dt_ms) / 1000.0f);
      speeds[speed_count++] = speed;
    }

    if (speed_count < 2) {
      return 0.0f;
    }

    float mean = 0.0f;
    for (size_t i = 0; i < speed_count; ++i) {
      mean += speeds[i];
    }
    mean /= static_cast<float>(speed_count);

    if (mean < 0.01f) {
      return 0.0f;
    }

    float variance = 0.0f;
    for (size_t i = 0; i < speed_count; ++i) {
      const float diff = speeds[i] - mean;
      variance += diff * diff;
    }
    variance /= static_cast<float>(speed_count);

    return sqrtf(variance) / mean;
  }

 private:
  const AnchorBeacon* findAnchor(DeviceRole role) const {
    for (size_t i = 0; i < anchor_count_; ++i) {
      if (anchors_[i].role == role) {
        return &anchors_[i];
      }
    }
    return nullptr;
  }

  void recordSample(const PositionSample& sample) {
    history_[history_head_] = sample;
    history_head_ = (history_head_ + 1) % config::POSITION_HISTORY_CAPACITY;
    if (history_count_ < config::POSITION_HISTORY_CAPACITY) {
      history_count_++;
    }
    latest_ = sample;
    has_fix_ = true;
  }

  AnchorBeacon anchors_[config::MAX_BEACON_ANCHORS]{};
  size_t anchor_count_ = 0;
  PositionSample history_[config::POSITION_HISTORY_CAPACITY]{};
  size_t history_head_ = 0;
  size_t history_count_ = 0;
  PositionSample latest_{};
  bool has_fix_ = false;
};

inline bool isInsideSafeZone(const PositionSample& position) {
  return position.x_m >= config::SAFE_ZONE_MIN_X_M &&
         position.x_m <= config::SAFE_ZONE_MAX_X_M &&
         position.y_m >= config::SAFE_ZONE_MIN_Y_M &&
         position.y_m <= config::SAFE_ZONE_MAX_Y_M;
}

inline DeviceRole roleFromBleName(const String& name) {
  if (name.indexOf("Waist") >= 0 || name.indexOf("waist") >= 0) {
    return DEVICE_WAIST_SAFETY_PAD;
  }
  if (name.indexOf("Shoe") >= 0 || name.indexOf("shoe") >= 0) {
    return DEVICE_SHOE_PAD;
  }
  if (name.indexOf("Stick") >= 0 || name.indexOf("stick") >= 0) {
    return DEVICE_WALKING_STICK;
  }
  return DEVICE_WAIST_SAFETY_PAD;
}
