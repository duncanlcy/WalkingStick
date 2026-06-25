#pragma once

#include <Arduino.h>

#include "protocol/packet.h"

namespace walkingstick {

uint16_t read_adc(uint8_t pin);
float read_battery_voltage(uint8_t pin, float divider_ratio = 2.0f);

class PressureSensor {
 public:
  void begin(uint8_t heel, uint8_t midfoot, uint8_t forefoot);
  bool read(protocol::PressureSample &sample);

 private:
  uint8_t heel_pin_ = 0;
  uint8_t midfoot_pin_ = 0;
  uint8_t forefoot_pin_ = 0;
};

}  // namespace walkingstick
