#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "config.h"
#include "device_roles.h"
#include "protocol.h"
#include "sensors.h"

static BLEScan* ble_scan = nullptr;
static bool alert_active = false;

class StickScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertised_device) override {
    if (!advertised_device.haveServiceUUID()) {
      return;
    }

  BLEUUID service_uuid(WALKING_STICK_SERVICE_UUID);
    if (!advertised_device.isAdvertisingService(service_uuid)) {
      return;
    }

    Serial.printf("Node detected: %s RSSI=%d\n",
                  advertised_device.getName().c_str(),
                  advertised_device.getRSSI());
  }
};

static void triggerLocalAlert(AlertType type) {
  alert_active = true;
  digitalWrite(pins::stick::VIBRATOR, HIGH);

  switch (type) {
    case ALERT_TYPE_FALL_DETECTED:
      Serial.println("ALERT: Fall detected — check wearer");
      break;
    case ALERT_TYPE_SOS:
      Serial.println("ALERT: SOS pressed");
      break;
    default:
      Serial.println("ALERT: Safety event");
      break;
  }
}

static void clearLocalAlert() {
  alert_active = false;
  digitalWrite(pins::stick::VIBRATOR, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(pins::stick::STATUS_LED, OUTPUT);
  pinMode(pins::stick::BUTTON_ALERT, INPUT_PULLUP);
  pinMode(pins::stick::VIBRATOR, OUTPUT);
  digitalWrite(pins::stick::VIBRATOR, LOW);

  BLEDevice::init(BLE_DEVICE_NAME);
  ble_scan = BLEDevice::getScan();
  ble_scan->setAdvertisedDeviceCallbacks(new StickScanCallbacks());
  ble_scan->setActiveScan(true);
  ble_scan->setInterval(100);
  ble_scan->setWindow(99);

  Serial.printf("[%s] walking stick ready\n", deviceRoleName(DEVICE_WALKING_STICK));
}

void loop() {
  static uint32_t last_scan = 0;
  static uint32_t last_battery_check = 0;

  if (digitalRead(pins::stick::BUTTON_ALERT) == LOW) {
    triggerLocalAlert(ALERT_TYPE_SOS);
    delay(300);
  }

  if (millis() - last_scan >= config::BLE_SCAN_INTERVAL_MS) {
  last_scan = millis();
    ble_scan->start(3, false);
  }

  if (millis() - last_battery_check >= 10000) {
    last_battery_check = millis();
    const uint8_t battery = readBatteryPercent(pins::stick::BATTERY_ADC);
    if (battery <= config::LOW_BATTERY_PERCENT) {
      Serial.printf("Low battery: %u%%\n", battery);
      triggerLocalAlert(ALERT_TYPE_LOW_BATTERY);
    } else if (alert_active) {
      clearLocalAlert();
    }
  }

  digitalWrite(pins::stick::STATUS_LED, alert_active ? HIGH : !digitalRead(pins::stick::STATUS_LED));
  delay(10);
}
