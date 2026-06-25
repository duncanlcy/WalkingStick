#include "shoe_app.h"

#include <BLEDevice.h>
#include <BLEUtils.h>

#include "common/config_pins.h"
#include "common/device_id.h"
#include "common/packet_builder.h"
#include "common/pressure_sensor.h"
#include "protocol/ble_uuids.h"
#include "protocol/packet.h"

namespace walkingstick {

namespace {

constexpr uint32_t kSampleIntervalMs = 50;

BLECharacteristic *telemetry_char = nullptr;
uint32_t last_sample_ms = 0;
PressureSensor pressure;
PacketBuilder packets;

void setup_ble_peripheral() {
  BLEDevice::init(DEVICE_NAME);
  BLEServer *server = BLEDevice::createServer();
  BLEService *service = server->createService(ble::kServiceUuid);
  telemetry_char = service->createCharacteristic(
      ble::kTelemetryUuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(ble::kServiceUuid);
  advertising->setScanResponse(true);
  advertising->start();
}

void publish_sample() {
  protocol::SensorPayload payload{};
  payload.imu = {};
  pressure.read(payload.pressure);
  payload.battery_v = read_battery_voltage(kBatteryAdcPin);
  payload.flags = 0;

  packets.reset();
  packets.set_type(protocol::PacketType::SensorSample);
  packets.set_payload(payload);
  packets.finalize();

  if (telemetry_char != nullptr) {
    telemetry_char->setValue(packets.data(), packets.size());
    telemetry_char->notify();
  }
}

}  // namespace

void shoe_pad_setup() {
  pinMode(kStatusLedPin, OUTPUT);
  analogReadResolution(12);
  pressure.begin(kFsHeelPin, kFsMidfootPin, kFsForefootPin);
  setup_ble_peripheral();
  Serial.printf("[%s] shoe pad ready\n", role_name());
}

void shoe_pad_loop() {
  const uint32_t now = millis();
  digitalWrite(kStatusLedPin, (now / 300) % 2);

  if (now - last_sample_ms >= kSampleIntervalMs) {
    last_sample_ms = now;
    publish_sample();
  }

  delay(5);
}

}  // namespace walkingstick
