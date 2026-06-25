#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "config.h"
#include "device_roles.h"
#include "protocol.h"
#include "sensors.h"
#include "safety.h"

static PressureSensor pressure;
static SafetyMonitor safety;
static BLEServer* ble_server = nullptr;
static BLECharacteristic* telemetry_char = nullptr;

static void publishTelemetry(const TelemetryPacket& packet) {
  if (!telemetry_char) {
    return;
  }

  uint8_t payload[sizeof(TelemetryPacket)];
  memcpy(payload, &packet, sizeof(packet));
  telemetry_char->setValue(payload, sizeof(payload));
  telemetry_char->notify();
}

static void setupBle() {
  BLEDevice::init(BLE_DEVICE_NAME);
  ble_server = BLEDevice::createServer();

  BLEService* service = ble_server->createService(WALKING_STICK_SERVICE_UUID);
  telemetry_char = service->createCharacteristic(
      TELEMETRY_CHAR_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  telemetry_char->addDescriptor(new BLE2902());

  service->start();
  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(WALKING_STICK_SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->start();
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(pins::shoe::STATUS_LED, OUTPUT);

  pressure.begin(
      pins::shoe::FSR_LEFT_HEEL,
      pins::shoe::FSR_LEFT_TOE,
      pins::shoe::FSR_RIGHT_HEEL,
      pins::shoe::FSR_RIGHT_TOE);

  setupBle();

  Serial.printf("[%s] shoe pad sensor ready\n", deviceRoleName(DEVICE_SHOE_PAD));
}

void loop() {
  static uint32_t last_sample = 0;

  if (millis() - last_sample < config::SENSOR_SAMPLE_MS) {
    return;
  }
  last_sample = millis();

  digitalWrite(pins::shoe::STATUS_LED, !digitalRead(pins::shoe::STATUS_LED));

  const PressureReading reading = pressure.read();
  const AlertEvent gait_alert = safety.evaluateGait(reading, DEVICE_SHOE_PAD);

  TelemetryPacket packet{};
  packet.protocol_version = PROTOCOL_VERSION;
  packet.source = DEVICE_SHOE_PAD;
  packet.sample.timestamp_ms = millis();
  packet.sample.pressure_left = reading.leftTotal();
  packet.sample.pressure_right = reading.rightTotal();
  packet.sample.battery_percent = 100;
  packet.sample.source = DEVICE_SHOE_PAD;
  packet.has_alert = gait_alert.level != ALERT_NONE;
  packet.alert = gait_alert;

  publishTelemetry(packet);

  if (packet.has_alert) {
    Serial.println(gait_alert.message);
  }
}
