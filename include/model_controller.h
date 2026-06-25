#pragma once

#include <Arduino.h>
#include <esp_system.h>

#include "config.h"
#include "protocol.h"

class ModelController {
 public:
  bool isRunning() const { return state_ == MODEL_STATE_RUNNING; }

  ModelState state() const { return state_; }

  void stop(const char* reason) {
    if (state_ == MODEL_STATE_RUNNING) {
      state_ = MODEL_STATE_STOPPED;
      stopped_at_ms_ = millis();
      strncpy(stop_reason_, reason, sizeof(stop_reason_) - 1);
      stop_reason_[sizeof(stop_reason_) - 1] = '\0';
    }
  }

  void start() {
    state_ = MODEL_STATE_RUNNING;
    stopped_at_ms_ = 0;
    recovery_reboot_attempted_ = false;
    stop_reason_[0] = '\0';
  }

  bool inferenceAllowed() const { return state_ == MODEL_STATE_RUNNING; }

  const char* stopReason() const { return stop_reason_; }

  void tick(bool sensor_healthy) {
    if (state_ != MODEL_STATE_STOPPED || !config::MODEL_AUTO_RECOVERY_REBOOT) {
      return;
    }

    if (sensor_healthy) {
      start();
      return;
    }

    if (recovery_reboot_attempted_) {
      return;
    }

    if (millis() - stopped_at_ms_ < config::MODEL_RECOVERY_REBOOT_MS) {
      return;
    }

    state_ = MODEL_STATE_RECOVERY;
    recovery_reboot_attempted_ = true;
    Serial.printf("Model stopped due to component failure (%s); rebooting for recovery\n",
                  stop_reason_);
    delay(100);
    esp_restart();
  }

  AlertEvent makeStoppedAlert(DeviceRole source) const {
    AlertEvent event{};
    event.timestamp_ms = millis();
    event.source = source;
    event.level = ALERT_WARNING;
    event.type = ALERT_TYPE_MODEL_STOPPED;
    snprintf(event.message, sizeof(event.message), "Model stopped: %s", stop_reason_);
    return event;
  }

  AlertEvent makeSensorFaultAlert(DeviceRole source, const char* reason) const {
    AlertEvent event{};
    event.timestamp_ms = millis();
    event.source = source;
    event.level = ALERT_WARNING;
    event.type = ALERT_TYPE_SENSOR_FAULT;
    snprintf(event.message, sizeof(event.message), "Sensor fault: %s", reason);
    return event;
  }

 private:
  ModelState state_ = MODEL_STATE_RUNNING;
  uint32_t stopped_at_ms_ = 0;
  bool recovery_reboot_attempted_ = false;
  char stop_reason_[64]{};
};
