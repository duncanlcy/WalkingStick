#pragma once

#include <Arduino.h>
#include "config.h"

struct AccelerometerReading {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float magnitude() const {
    return sqrtf(x * x + y * y + z * z);
  }
};

struct PressureReading {
  uint16_t left_heel = 0;
  uint16_t left_toe = 0;
  uint16_t right_heel = 0;
  uint16_t right_toe = 0;

  uint16_t leftTotal() const { return left_heel + left_toe; }
  uint16_t rightTotal() const { return right_heel + right_toe; }
  uint16_t total() const { return leftTotal() + rightTotal(); }
};

class AccelerometerSensor {
 public:
  void begin(int int_pin) {
    pinMode(int_pin, INPUT);
    // MPU6050 / BMI160 init would go here in hardware integration
  }

  AccelerometerReading read() {
    // Placeholder: replace with I2C driver when hardware is wired
    AccelerometerReading r;
    r.x = 0.0f;
    r.y = 0.0f;
    r.z = 1.0f;  // 1g at rest
    return r;
  }
};

class PressureSensor {
 public:
  void begin(int left_heel, int left_toe, int right_heel, int right_toe) {
    pins_[0] = left_heel;
    pins_[1] = left_toe;
    pins_[2] = right_heel;
    pins_[3] = right_toe;
    for (int pin : pins_) {
      pinMode(pin, INPUT);
    }
  }

  PressureReading read() {
    PressureReading r;
    r.left_heel = analogRead(pins_[0]);
    r.left_toe = analogRead(pins_[1]);
    r.right_heel = analogRead(pins_[2]);
    r.right_toe = analogRead(pins_[3]);
    return r;
  }

 private:
  int pins_[4] = {};
};

inline uint8_t readBatteryPercent(int adc_pin) {
  int raw = analogRead(adc_pin);
  // Simple linear map for 3.3V LiPo divider — calibrate per board
  float voltage = (raw / 4095.0f) * 3.3f * 2.0f;
  if (voltage >= 4.2f) return 100;
  if (voltage <= 3.3f) return 0;
  return static_cast<uint8_t>((voltage - 3.3f) / (4.2f - 3.3f) * 100.0f);
}
