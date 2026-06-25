#pragma once

#include "protocol/packet.h"

namespace walkingstick {

inline protocol::DeviceRole current_role() {
#if defined(ROLE_WAIST_HUB)
  return protocol::DeviceRole::WaistHub;
#elif defined(ROLE_WALKING_STICK)
  return protocol::DeviceRole::WalkingStick;
#elif defined(SHOE_SIDE_LEFT)
  return protocol::DeviceRole::ShoePadLeft;
#elif defined(SHOE_SIDE_RIGHT)
  return protocol::DeviceRole::ShoePadRight;
#else
  return protocol::DeviceRole::WaistHub;
#endif
}

inline const char *role_name() {
  switch (current_role()) {
    case protocol::DeviceRole::WaistHub:
      return "waist_hub";
    case protocol::DeviceRole::WalkingStick:
      return "walking_stick";
    case protocol::DeviceRole::ShoePadLeft:
      return "shoe_pad_left";
    case protocol::DeviceRole::ShoePadRight:
      return "shoe_pad_right";
    default:
      return "unknown";
  }
}

}  // namespace walkingstick
