#include "common/packet_builder.h"
#include "common/device_id.h"

namespace walkingstick {

protocol::DeviceRole PacketBuilder::current_role() {
  return walkingstick::current_role();
}

}  // namespace walkingstick
