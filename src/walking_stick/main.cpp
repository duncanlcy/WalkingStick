#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "config.h"
#include "device_roles.h"
#include "protocol.h"
#include "sensors.h"
#include "safety.h"
#include "location.h"

static BLEScan* ble_scan = nullptr;
static bool alert_active = false;
static StickLocationTracker location_tracker;
static SafetyMonitor safety;
static AccelerometerSensor accelerometer;

static BeaconReading beacon_buffer[config::MAX_BEACON_ANCHORS];
static size_t beacon_count = 0;

class StickScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertised_device) override {
    if (!advertised_device.haveServiceUUID()) {
      return;
    }

    BLEUUID service_uuid(WALKING_STICK_SERVICE_UUID);
    if (!advertised_device.isAdvertisingService(service_uuid)) {
      return;
    }

    const DeviceRole role = roleFromBleName(advertised_device.getName().c_str());
    if (beacon_count < config::MAX_BEACON_ANCHORS) {
      beacon_buffer[beacon_count++] = {role, static_cast<int8_t>(advertised_device.getRSSI())};
    }

    Serial.printf("Node detected: %s RSSI=%d\n",
                  advertised_device.getName().c_str(),
                  advertised_device.getRSSI());
  }
};

static void triggerLocalAlert(const AlertEvent& alert) {
  alert_active = true;
  digitalWrite(pins::stick::VIBRATOR, HIGH);
  Serial.printf("ALERT [%u]: %s\n", alert.type, alert.message);
}

static void clearLocalAlert() {
  alert_active = false;
  digitalWrite(pins::stick::VIBRATOR, LOW);
}

static void publishAlertIfNeeded(const AlertEvent& alert) {
  if (alert.level == ALERT_NONE) {
    return;
  }
  triggerLocalAlert(alert);
}

static void traceStickLocation() {
  if (beacon_count == 0) {
    return;
  }

  const PositionSample position =
      location_tracker.estimatePosition(beacon_buffer, beacon_count);
  beacon_count = 0;

  if (!location_tracker.hasFix()) {
    return;
  }

  Serial.printf("Stick position: (%.2f, %.2f) m accuracy=%.2f m\n",
                position.x_m, position.y_m, position.accuracy_m);

  const AlertEvent position_alert =
      safety.evaluatePosition(position, DEVICE_WALKING_STICK);
  const AlertEvent cognitive_alert =
      safety.evaluateCognitiveMovement(location_tracker, DEVICE_WALKING_STICK);

  if (position_alert.level != ALERT_NONE) {
    publishAlertIfNeeded(position_alert);
  } else if (cognitive_alert.level != ALERT_NONE) {
    publishAlertIfNeeded(cognitive_alert);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(pins::stick::STATUS_LED, OUTPUT);
  pinMode(pins::stick::BUTTON_ALERT, INPUT_PULLUP);
  pinMode(pins::stick::VIBRATOR, OUTPUT);
  digitalWrite(pins::stick::VIBRATOR, LOW);

  accelerometer.begin(pins::stick::STATUS_LED);

  BLEDevice::init(BLE_DEVICE_NAME);
  ble_scan = BLEDevice::getScan();
  ble_scan->setAdvertisedDeviceCallbacks(new StickScanCallbacks());
  ble_scan->setActiveScan(true);
  ble_scan->setInterval(100);
  ble_scan->setWindow(99);

  Serial.printf("[%s] walking stick ready — location tracing enabled\n",
                deviceRoleName(DEVICE_WALKING_STICK));
}

void loop() {
  static uint32_t last_scan = 0;
  static uint32_t last_battery_check = 0;
  static uint32_t last_posture_check = 0;

  if (digitalRead(pins::stick::BUTTON_ALERT) == LOW) {
    AlertEvent sos{};
    sos.timestamp_ms = millis();
    sos.level = ALERT_CRITICAL;
    sos.type = ALERT_TYPE_SOS;
    sos.source = DEVICE_WALKING_STICK;
    snprintf(sos.message, sizeof(sos.message), "SOS pressed");
    triggerLocalAlert(sos);
    delay(300);
  }

  if (millis() - last_scan >= config::BLE_SCAN_INTERVAL_MS) {
    last_scan = millis();
    ble_scan->start(3, false);
    traceStickLocation();
  }

  if (millis() - last_posture_check >= config::POSITION_SAMPLE_MS) {
    last_posture_check = millis();
    const AccelerometerReading accel = accelerometer.read();
    const AlertEvent posture_alert =
        safety.evaluateStickPosture(accel, DEVICE_WALKING_STICK);
    publishAlertIfNeeded(posture_alert);
  }

  if (millis() - last_battery_check >= 10000) {
    last_battery_check = millis();
    const uint8_t battery = readBatteryPercent(pins::stick::BATTERY_ADC);
    if (battery <= config::LOW_BATTERY_PERCENT) {
      Serial.printf("Low battery: %u%%\n", battery);
      AlertEvent low_batt{};
      low_batt.timestamp_ms = millis();
      low_batt.level = ALERT_WARNING;
      low_batt.type = ALERT_TYPE_LOW_BATTERY;
      low_batt.source = DEVICE_WALKING_STICK;
      snprintf(low_batt.message, sizeof(low_batt.message), "Low battery: %u%%", battery);
      triggerLocalAlert(low_batt);
    } else if (alert_active) {
      clearLocalAlert();
    }
  }

  digitalWrite(pins::stick::STATUS_LED, alert_active ? HIGH : !digitalRead(pins::stick::STATUS_LED));
  delay(10);
}
