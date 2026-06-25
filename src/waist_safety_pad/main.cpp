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
#include "media_recommendations.h"

static AccelerometerSensor accelerometer;
static SafetyMonitor safety;
static DataLogger logger;
static MediaRecommendationEngine recommender;
static BLEServer* ble_server = nullptr;
static BLECharacteristic* telemetry_char = nullptr;
static BLECharacteristic* alert_char = nullptr;
static BLECharacteristic* command_char = nullptr;
static BLECharacteristic* media_char = nullptr;
static bool gait_irregular_active = false;

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

static void publishRecommendations() {
  if (!media_char) {
    return;
  }

  const uint32_t hour = (millis() / 3600000UL + 8) % 24;
  const MediaRecommendationList list =
      recommender.recommend(hour, gait_irregular_active);

  uint8_t payload[sizeof(MediaRecommendationList)];
  memcpy(payload, &list, sizeof(list));
  media_char->setValue(payload, sizeof(payload));
  media_char->notify();

  Serial.printf("Sent %u recommendations: %s\n", list.count, list.greeting);
}

static void handleMediaCommand(const MediaCommand& cmd) {
  switch (cmd.command) {
    case MEDIA_CMD_REQUEST_RECOMMENDATIONS:
      if (cmd.preference != PREF_NONE) {
        recommender.setPreference(cmd.preference);
      }
      publishRecommendations();
      break;

    case MEDIA_CMD_SET_PREFERENCE:
      recommender.setPreference(cmd.preference);
      Serial.printf("Preference updated: %s\n",
                    recommender.preferenceLabel(cmd.preference));
      publishRecommendations();
      break;

    case MEDIA_CMD_SELECT_RECOMMENDATION:
      Serial.printf("Selected recommendation #%u\n", cmd.selection_index + 1);
      break;

    case MEDIA_CMD_PLAY_PAUSE:
    case MEDIA_CMD_NEXT:
    case MEDIA_CMD_PREVIOUS:
    case MEDIA_CMD_VOLUME_UP:
    case MEDIA_CMD_VOLUME_DOWN:
      Serial.printf("Media command from stick: %u\n", cmd.command);
      break;

    default:
      break;
  }
}

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    const std::string value = characteristic->getValue();
    if (value.size() < sizeof(MediaCommand)) {
      return;
    }

    MediaCommand cmd{};
    memcpy(&cmd, value.data(), sizeof(cmd));
    handleMediaCommand(cmd);
  }
};

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
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  command_char->setCallbacks(new CommandCallbacks());

  media_char = service->createCharacteristic(
      MEDIA_CHAR_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  media_char->addDescriptor(new BLE2902());

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

  Serial.printf("[%s] waist safety pad ready (media recommendations enabled)\n",
                deviceRoleName(DEVICE_WAIST_SAFETY_PAD));
}

void loop() {
  static uint32_t last_sample = 0;

  if (millis() - last_sample < config::SENSOR_SAMPLE_MS) {
    return;
  }
  last_sample = millis();

  digitalWrite(pins::waist::STATUS_LED, !digitalRead(pins::waist::STATUS_LED));

  const AccelerometerReading accel = accelerometer.read();
  const AlertEvent safety_alert = safety.evaluate(accel, DEVICE_WAIST_SAFETY_PAD);

  gait_irregular_active =
      safety_alert.type == ALERT_TYPE_GAIT_IRREGULAR &&
      safety_alert.level >= ALERT_WARNING;

  TelemetryPacket packet{};
  packet.protocol_version = PROTOCOL_VERSION;
  packet.source = DEVICE_WAIST_SAFETY_PAD;
  packet.sample.timestamp_ms = millis();
  packet.sample.accel_x = accel.x;
  packet.sample.accel_y = accel.y;
  packet.sample.accel_z = accel.z;
  packet.sample.battery_percent = 100;
  packet.sample.source = DEVICE_WAIST_SAFETY_PAD;
  packet.has_alert = safety_alert.level != ALERT_NONE;
  packet.alert = safety_alert;

  logger.log(packet);
  publishTelemetry(packet);

  if (packet.has_alert) {
    publishAlert(safety_alert);
    Serial.println(safety_alert.message);
  }
}
