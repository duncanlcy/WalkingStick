#include "hub_app.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <FS.h>
#include <SPIFFS.h>

#include "common/config_pins.h"
#include "common/device_id.h"
#include "common/imu_sensor.h"
#include "common/packet_builder.h"
#include "common/pressure_sensor.h"
#include "protocol/ble_uuids.h"
#include "protocol/packet.h"

namespace walkingstick {

namespace {

constexpr uint32_t kSampleIntervalMs = 200;
constexpr uint32_t kLogFlushIntervalMs = 5000;
constexpr const char *kLogPath = "/telemetry.csv";

BLEServer *server = nullptr;
bool client_connected = false;
uint32_t last_sample_ms = 0;
uint32_t last_flush_ms = 0;
ImuSensor imu;
PacketBuilder packets;
File log_file;

class HubCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer * /*srv*/) override { client_connected = true; }
  void onDisconnect(BLEServer * /*srv*/) override {
    client_connected = false;
    BLEDevice::startAdvertising();
  }
};

void ensure_log_header() {
  if (SPIFFS.exists(kLogPath)) {
    return;
  }
  log_file = SPIFFS.open(kLogPath, FILE_WRITE);
  if (!log_file) {
    return;
  }
  log_file.println("timestamp_ms,role,accel_x,accel_y,accel_z,heel,midfoot,forefoot,battery_v");
  log_file.close();
}

void append_log(const protocol::PacketHeader &header, const protocol::SensorPayload &payload) {
  log_file = SPIFFS.open(kLogPath, FILE_APPEND);
  if (!log_file) {
    return;
  }
  log_file.printf("%lu,%u,%.3f,%.3f,%.3f,%u,%u,%u,%.2f\n", header.timestamp_ms,
                  header.role, payload.imu.accel_x, payload.imu.accel_y, payload.imu.accel_z,
                  payload.pressure.heel, payload.pressure.midfoot, payload.pressure.forefoot,
                  payload.battery_v);
  log_file.close();
}

void publish_alert(protocol::AlertLevel level, uint8_t code, const char *message) {
  packets.reset();
  packets.set_type(protocol::PacketType::Alert);
  protocol::AlertPayload alert{};
  alert.level = static_cast<uint8_t>(level);
  alert.code = code;
  strncpy(alert.message, message, sizeof(alert.message) - 1);
  packets.set_payload(alert);
  packets.finalize();
  digitalWrite(kBuzzerPin, level >= protocol::AlertLevel::Warning ? HIGH : LOW);
}

void handle_incoming_packet(const uint8_t *data, size_t len) {
  if (len < sizeof(protocol::PacketHeader)) {
    return;
  }

  protocol::PacketHeader header{};
  memcpy(&header, data, sizeof(header));
  if (!protocol::validate_header(header)) {
    return;
  }

  if (header.type != static_cast<uint8_t>(protocol::PacketType::SensorSample)) {
    return;
  }

  protocol::SensorPayload payload{};
  memcpy(&payload, data + sizeof(header), sizeof(payload));
  append_log(header, payload);

  if (abs(payload.imu.accel_x) > 2.5f || abs(payload.imu.accel_y) > 2.5f) {
    publish_alert(protocol::AlertLevel::Warning, 1, "Sudden tilt detected");
  }
}

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    const std::string value = characteristic->getValue();
    handle_incoming_packet(reinterpret_cast<const uint8_t *>(value.data()), value.size());
  }
};

void setup_ble_hub() {
  BLEDevice::init(DEVICE_NAME);
  server = BLEDevice::createServer();
  server->setCallbacks(new HubCallbacks());

  BLEService *service = server->createService(ble::kServiceUuid);
  service->createCharacteristic(ble::kTelemetryUuid, BLECharacteristic::PROPERTY_NOTIFY);
  service->createCharacteristic(ble::kCommandUuid,
                                  BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR)
      ->setCallbacks(new CommandCallbacks());
  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(ble::kServiceUuid);
  advertising->setScanResponse(true);
  advertising->start();
}

void sample_hub() {
  protocol::SensorPayload payload{};
  imu.read(payload.imu);
  payload.pressure = {};
  payload.battery_v = read_battery_voltage(kBatteryAdcPin);
  payload.flags = 0;

  packets.reset();
  packets.set_type(protocol::PacketType::SensorSample);
  packets.set_payload(payload);
  packets.finalize();

  protocol::PacketHeader header{};
  memcpy(&header, packets.data(), sizeof(header));
  append_log(header, payload);
}

}  // namespace

void waist_hub_setup() {
  pinMode(kStatusLedPin, OUTPUT);
  pinMode(kBuzzerPin, OUTPUT);
  digitalWrite(kBuzzerPin, LOW);

  analogReadResolution(12);
  SPIFFS.begin(true);
  ensure_log_header();
  imu.begin(kImuSdaPin, kImuSclPin);
  setup_ble_hub();

  Serial.printf("[%s] waist safety pad ready\n", role_name());
}

void waist_hub_loop() {
  const uint32_t now = millis();
  digitalWrite(kStatusLedPin, client_connected ? HIGH : LOW);

  if (now - last_sample_ms >= kSampleIntervalMs) {
    last_sample_ms = now;
    sample_hub();
  }

  if (now - last_flush_ms >= kLogFlushIntervalMs) {
    last_flush_ms = now;
    Serial.printf("[%s] logging active, free heap %u\n", role_name(), ESP.getFreeHeap());
  }

  delay(5);
}

}  // namespace walkingstick
