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
#include "data_logger.h"
#include "component_health.h"
#include "model_controller.h"

static AccelerometerSensor accelerometer;
static SafetyMonitor safety;
static DataLogger logger;
static ComponentHealthMonitor health_monitor;
static ModelController model;
static BLEServer* ble_server = nullptr;
static BLECharacteristic* telemetry_char = nullptr;
static BLECharacteristic* alert_char = nullptr;
static BLECharacteristic* command_char = nullptr;
static bool fault_alert_sent = false;
static bool model_stopped_alert_sent = false;

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    const std::string& value = characteristic->getValue();
    if (value.size() < sizeof(CommandPacket)) {
      return;
    }

    CommandPacket command{};
    memcpy(&command, value.data(), sizeof(command));

    switch (command.type) {
      case CMD_START_MODEL:
        model.start();
        fault_alert_sent = false;
        model_stopped_alert_sent = false;
        Serial.println("Model inference restarted via command");
        break;
      case CMD_STOP_MODEL:
        model.stop("stopped via command");
        Serial.println("Model inference stopped via command");
        break;
      case CMD_REBOOT_DEVICE:
        Serial.println("Reboot requested via command");
        delay(100);
        esp_restart();
        break;
      default:
        break;
    }
  }
};

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

  command_char = service->createCharacteristic(
      COMMAND_CHAR_UUID,
      BLECharacteristic::PROPERTY_WRITE);
  command_char->setCallbacks(new CommandCallbacks());

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

  if (millis() - last_sample < config::SENSOR_SAMPLE_MS) {
    return;
  }
  last_sample = millis();

  digitalWrite(pins::waist::STATUS_LED, !digitalRead(pins::waist::STATUS_LED));

  const AccelerometerReading accel = accelerometer.read();
  const SensorHealth sensor_health = health_monitor.checkAccelerometer(accel);
  const bool sensor_healthy = health_monitor.accelerometerHealthy();

  if (!sensor_healthy && model.inferenceAllowed()) {
    model.stop(sensor_health.reason);
    fault_alert_sent = false;
    model_stopped_alert_sent = false;
    Serial.printf("Stopping model inference: %s\n", sensor_health.reason);
  }

  model.tick(sensor_healthy);

  AlertEvent active_alert{};
  if (!sensor_healthy && !fault_alert_sent) {
    active_alert = model.makeSensorFaultAlert(DEVICE_WAIST_SAFETY_PAD, sensor_health.reason);
    fault_alert_sent = true;
  } else if (!model.inferenceAllowed() && !model_stopped_alert_sent) {
    active_alert = model.makeStoppedAlert(DEVICE_WAIST_SAFETY_PAD);
    model_stopped_alert_sent = true;
  } else if (model.inferenceAllowed()) {
    active_alert = safety.evaluate(accel, DEVICE_WAIST_SAFETY_PAD);
  }

  TelemetryPacket packet{};
  packet.protocol_version = PROTOCOL_VERSION;
  packet.source = DEVICE_WAIST_SAFETY_PAD;
  packet.sample.timestamp_ms = millis();
  packet.sample.accel_x = accel.x;
  packet.sample.accel_y = accel.y;
  packet.sample.accel_z = accel.z;
  packet.sample.battery_percent = 100;
  packet.sample.source = DEVICE_WAIST_SAFETY_PAD;
  packet.model_state = model.state();
  packet.sensor_healthy = sensor_healthy;
  packet.has_alert = active_alert.level != ALERT_NONE;
  packet.alert = active_alert;

  logger.log(packet);
  publishTelemetry(packet);

  if (packet.has_alert) {
    publishAlert(active_alert);
    Serial.println(active_alert.message);
  }
}
