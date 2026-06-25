#pragma once

#include <Arduino.h>

#include "protocol/packet.h"

namespace walkingstick {

class PacketBuilder {
 public:
  PacketBuilder() { reset(); }

  void reset() {
    header_.magic0 = protocol::kMagic0;
    header_.magic1 = protocol::kMagic1;
    header_.version = protocol::kProtocolVersion;
    header_.role = static_cast<uint8_t>(current_role());
    header_.sequence = sequence_++;
    header_.timestamp_ms = millis();
    payload_len_ = 0;
  }

  void set_type(protocol::PacketType type) {
    header_.type = static_cast<uint8_t>(type);
  }

  template <typename T>
  bool set_payload(const T &payload) {
    static_assert(sizeof(T) <= protocol::kMaxPayload, "payload too large");
    memcpy(payload_, &payload, sizeof(T));
    payload_len_ = sizeof(T);
    return true;
  }

  uint8_t *data() { return buffer_; }

  const uint8_t *data() const { return buffer_; }

  size_t size() const {
    return sizeof(protocol::PacketHeader) + payload_len_;
  }

  void finalize() {
    header_.payload_len = payload_len_;
    memcpy(buffer_, &header_, sizeof(protocol::PacketHeader));
    if (payload_len_ > 0) {
      memcpy(buffer_ + sizeof(protocol::PacketHeader), payload_, payload_len_);
    }
  }

 private:
  static protocol::DeviceRole current_role();

  uint32_t sequence_ = 0;
  protocol::PacketHeader header_{};
  uint8_t payload_[protocol::kMaxPayload]{};
  uint16_t payload_len_ = 0;
  mutable uint8_t buffer_[sizeof(protocol::PacketHeader) + protocol::kMaxPayload]{};
};

}  // namespace walkingstick
