#include "common/imu_sensor.h"

#include <Wire.h>

namespace walkingstick {

bool ImuSensor::begin(int sda_pin, int scl_pin) {
  sda_pin_ = sda_pin;
  scl_pin_ = scl_pin;
  Wire.begin(sda_pin_, scl_pin_);
  ready_ = true;
  return ready_;
}

bool ImuSensor::read(protocol::ImuSample &sample) {
  if (!ready_) {
    return false;
  }

  // Placeholder values until a physical IMU (MPU6050 / BMI270) is wired.
  // Replace with driver reads in imu_sensor.cpp when hardware is connected.
  sample.accel_x = 0.0f;
  sample.accel_y = 0.0f;
  sample.accel_z = 1.0f;
  sample.gyro_x = 0.0f;
  sample.gyro_y = 0.0f;
  sample.gyro_z = 0.0f;
  return true;
}

}  // namespace walkingstick
