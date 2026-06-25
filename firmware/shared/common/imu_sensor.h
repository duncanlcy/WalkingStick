#pragma once

#include <Arduino.h>

#include "protocol/packet.h"

namespace walkingstick {

class ImuSensor {
 public:
  bool begin(int sda_pin, int scl_pin);
  bool read(protocol::ImuSample &sample);

 private:
  int sda_pin_ = -1;
  int scl_pin_ = -1;
  bool ready_ = false;
};

}  // namespace walkingstick
