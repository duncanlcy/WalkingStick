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
#include "location.h"
#include "data_logger.h"

static AccelerometerSensor accelerometer;
static SafetyMonitor safety;
static DataLogger logger;
static StickLocationTracker location_tracker;
static BLEServer* ble_server = nullptr;
static BLECharacteristic* telemetry_char = nullptr;
static BLECharacteristic* alert_char = nullptr;

static void publishTelemetry(const TelemetryPacket& packet) {
  if (!telemetry_char) {
    return;
  }

  uint8_t payload[sizeof(TelemetryPacket)];
  memcpy(payload, &packet, sizeof(packet));
  telemetry_char->setValue(payload, sizeof(payload));
  telemetry_char->notify();
}

static void publishAlert(const AlertEvent& alert) {
  if (!alert_char || alert.level == ALERT_NONE) {
    return;
  }

  uint8_t payload[sizeof(AlertEvent)];
  memcpy(payload, &alert, sizeof(alert));
  alert_char->setValue(payload, sizeof(payload));
  alert_char->notify();

  if (alert.level >= ALERT_WARNING) {
    digitalWrite(pins::waist::BUZZER, HIGH);
    delay(100);
    digitalWrite(pins::waist::BUZZER, LOW);
  }
}

static void setupBle() {
  BLEDevice::init(BLE_DEVICE_NAME);
  ble_server = BLEDevice::createServer();

  BLEService* service = ble_server->createService(WALKING_STICK_SERVICE_UUID);
  telemetry_char = service->createCharacteristic(
      TELEMETRY_CHAR_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  telemetry_char->addDescriptor(new BLE2902());

  alert_char = service->createCharacteristic(
      ALERT_CHAR_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  alert_char->addDescriptor(new BLE2902());

  service->start();
  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(WALKING_STICK_SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->start();
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(pins::waist::STATUS_LED, OUTPUT);
  pinMode(pins::waist::BUZZER, OUTPUT);
  pinMode(pins::waist::SD_CS, OUTPUT);
  digitalWrite(pins::waist::SD_CS, HIGH);

  accelerometer.begin(pins::waist::MPU_INT);
  setupBle();

  Serial.printf("[%s] waist safety pad ready\n", deviceRoleName(DEVICE_WAIST_SAFETY_PAD));
}

void loop() {
  static uint32_t last_sample = 0;
  static uint32_t last_position_check = 0;

  if (millis() - last_sample < config::SENSOR_SAMPLE_MS) {
    return;
  }
  last_sample = millis();

  digitalWrite(pins::waist::STATUS_LED, !digitalRead(pins::waist::STATUS_LED));

  const AccelerometerReading accel = accelerometer.read();
  const AlertEvent safety_alert = safety.evaluate(accel, DEVICE_WAIST_SAFETY_PAD);

  TelemetryPacket packet{};
  packet.protocol_version = PROTOCOL_VERSION;
  packet.source = DEVICE_WAIST_SAFETY_PAD;
  packet.sample.timestamp_ms = millis();
  packet.sample.accel_x = accel.x;
  packet.sample.accel_y = accel.y;
  packet.sample.accel_z = accel.z;
  packet.sample.battery_percent = 100;
  packet.sample.source = DEVICE_WAIST_SAFETY_PAD;
  packet.sample.has_position = false;
  packet.has_alert = safety_alert.level != ALERT_NONE;
  packet.alert = safety_alert;

  logger.log(packet);
  publishTelemetry(packet);

  if (packet.has_alert) {
    publishAlert(safety_alert);
    Serial.println(safety_alert.message);
  }

  if (millis() - last_position_check >= config::POSITION_SAMPLE_MS) {
    last_position_check = millis();

    BeaconReading anchor_reading{DEVICE_WAIST_SAFETY_PAD, config::RSSI_AT_1M_DBM};
    const PositionSample position =
        location_tracker.estimatePosition(&anchor_reading, 1);

    if (location_tracker.hasFix()) {
      packet.sample.position = position;
      packet.sample.has_position = true;

      const AlertEvent position_alert =
          safety.evaluatePosition(position, DEVICE_WAIST_SAFETY_PAD);
      const AlertEvent cognitive_alert =
          safety.evaluateCognitiveMovement(location_tracker, DEVICE_WAIST_SAFETY_PAD);

      AlertEvent combined = position_alert;
      if (cognitive_alert.level > combined.level) {
        combined = cognitive_alert;
      }

      if (combined.level != ALERT_NONE) {
        publishAlert(combined);
        Serial.println(combined.message);
      }
    }
  }
}
