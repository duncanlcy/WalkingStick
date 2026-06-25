#pragma once

#include <Arduino.h>
#include "config.h"

enum ButtonAction : uint8_t {
  BUTTON_NONE = 0,
  BUTTON_SHORT_PRESS,
  BUTTON_LONG_PRESS,
};

struct ButtonEvent {
  uint8_t button_id;
  ButtonAction action;
};

// Debounced, elderly-friendly button handler with distinct short/long press windows.
class ElderlyInputHandler {
 public:
  void begin(const int* pins, uint8_t count) {
    button_count_ = count < kMaxButtons ? count : kMaxButtons;
    for (uint8_t i = 0; i < button_count_; ++i) {
      pins_[i] = pins[i];
      pinMode(pins_[i], INPUT_PULLUP);
      last_reading_[i] = HIGH;
      stable_reading_[i] = HIGH;
      last_change_ms_[i] = 0;
      press_start_ms_[i] = 0;
      long_press_sent_[i] = false;
    }
  }

  bool poll(ButtonEvent* event) {
    const uint32_t now = millis();

    for (uint8_t i = 0; i < button_count_; ++i) {
      const int reading = digitalRead(pins_[i]);

      if (reading != last_reading_[i]) {
        last_change_ms_[i] = now;
        last_reading_[i] = reading;
      }

      if ((now - last_change_ms_[i]) < config::BUTTON_DEBOUNCE_MS) {
        continue;
      }

      if (reading != stable_reading_[i]) {
        stable_reading_[i] = reading;

        if (reading == LOW) {
          press_start_ms_[i] = now;
          long_press_sent_[i] = false;
        } else if (!long_press_sent_[i]) {
          event->button_id = i;
          event->action = BUTTON_SHORT_PRESS;
          return true;
        }
      }

      if (stable_reading_[i] == LOW && !long_press_sent_[i] &&
          (now - press_start_ms_[i]) >= config::BUTTON_LONG_PRESS_MS) {
        long_press_sent_[i] = true;
        event->button_id = i;
        event->action = BUTTON_LONG_PRESS;
        return true;
      }
    }

    return false;
  }

 private:
  static constexpr uint8_t kMaxButtons = 8;

  uint8_t button_count_ = 0;
  int pins_[kMaxButtons]{};
  int last_reading_[kMaxButtons]{};
  int stable_reading_[kMaxButtons]{};
  uint32_t last_change_ms_[kMaxButtons]{};
  uint32_t press_start_ms_[kMaxButtons]{};
  bool long_press_sent_[kMaxButtons]{};
};
