#pragma once

#include <cstdint>

namespace walkingstick::protocol {

constexpr uint8_t kMagic0 = 0x57;  // 'W'
constexpr uint8_t kMagic1 = 0x53;  // 'S'
constexpr uint8_t kProtocolVersion = 1;

enum class DeviceRole : uint8_t {
  WaistHub = 0,
  WalkingStick = 1,
  ShoePadLeft = 2,
  ShoePadRight = 3,
};

enum class PacketType : uint8_t {
  Heartbeat = 0x01,
  SensorSample = 0x02,
  Alert = 0x03,
  Config = 0x04,
};

enum class AlertLevel : uint8_t {
  Info = 0,
  Warning = 1,
  Critical = 2,
};

#pragma pack(push, 1)

struct PacketHeader {
  uint8_t magic0;
  uint8_t magic1;
  uint8_t version;
  uint8_t type;
  uint8_t role;
  uint32_t sequence;
  uint32_t timestamp_ms;
  uint16_t payload_len;
};

struct ImuSample {
  float accel_x;
  float accel_y;
  float accel_z;
  float gyro_x;
  float gyro_y;
  float gyro_z;
};

struct PressureSample {
  uint16_t heel;
  uint16_t midfoot;
  uint16_t forefoot;
  uint16_t total;
};

struct SensorPayload {
  ImuSample imu;
  PressureSample pressure;
  float battery_v;
  uint8_t flags;
};

struct AlertPayload {
  uint8_t level;
  uint8_t code;
  char message[48];
};

constexpr uint16_t kMaxPayload =
    sizeof(SensorPayload) > sizeof(AlertPayload) ? sizeof(SensorPayload) : sizeof(AlertPayload);

#pragma pack(pop)

inline bool validate_header(const PacketHeader &header) {
  return header.magic0 == kMagic0 && header.magic1 == kMagic1 &&
         header.version == kProtocolVersion &&
         header.payload_len <= kMaxPayload;
}

}  // namespace walkingstick::protocol
