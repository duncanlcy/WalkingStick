#include "common/pressure_sensor.h"

namespace walkingstick {

uint16_t read_adc(uint8_t pin) {
  return static_cast<uint16_t>(analogRead(pin));
}

float read_battery_voltage(uint8_t pin, float divider_ratio) {
  const float adc_v = (analogRead(pin) / 4095.0f) * 3.3f;
  return adc_v * divider_ratio;
}

void PressureSensor::begin(uint8_t heel, uint8_t midfoot, uint8_t forefoot) {
  heel_pin_ = heel;
  midfoot_pin_ = midfoot;
  forefoot_pin_ = forefoot;
  pinMode(heel_pin_, INPUT);
  pinMode(midfoot_pin_, INPUT);
  pinMode(forefoot_pin_, INPUT);
}

bool PressureSensor::read(protocol::PressureSample &sample) {
  sample.heel = read_adc(heel_pin_);
  sample.midfoot = read_adc(midfoot_pin_);
  sample.forefoot = read_adc(forefoot_pin_);
  sample.total = static_cast<uint16_t>(
      (static_cast<uint32_t>(sample.heel) + sample.midfoot + sample.forefoot) / 3);
  return true;
}

}  // namespace walkingstick
