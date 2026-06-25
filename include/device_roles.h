#pragma once

enum DeviceRole : uint8_t {
  DEVICE_WAIST_SAFETY_PAD = 0,
  DEVICE_SHOE_PAD = 1,
  DEVICE_WALKING_STICK = 2,
};

#ifndef DEVICE_ROLE
#define DEVICE_ROLE DEVICE_WAIST_SAFETY_PAD
#endif

inline const char* deviceRoleName(DeviceRole role) {
  switch (role) {
    case DEVICE_WAIST_SAFETY_PAD: return "waist_safety_pad";
    case DEVICE_SHOE_PAD: return "shoe_pad";
    case DEVICE_WALKING_STICK: return "walking_stick";
    default: return "unknown";
  }
}
