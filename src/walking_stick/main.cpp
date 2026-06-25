#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>

#include "config.h"
#include "device_roles.h"
#include "protocol.h"
#include "sensors.h"
#include "elderly_input.h"
#include "media_player.h"

static constexpr uint8_t BTN_PLAY = 0;
static constexpr uint8_t BTN_NEXT = 1;
static constexpr uint8_t BTN_VOLUME = 2;
static constexpr uint8_t BTN_RECOMMEND = 3;

static BLEScan* ble_scan = nullptr;
static BLEClient* hub_client = nullptr;
static BLERemoteCharacteristic* command_char = nullptr;
static BLERemoteCharacteristic* media_char = nullptr;
static bool alert_active = false;
static bool hub_connected = false;
static bool scan_in_progress = false;

static ElderlyInputHandler input_handler;
static MediaPlayer media_player;
static ElderlyPreference active_preference = PREF_NONE;

static void hapticPulse(uint16_t duration_ms) {
  digitalWrite(pins::stick::VIBRATOR, HIGH);
  delay(duration_ms);
  digitalWrite(pins::stick::VIBRATOR, LOW);
}

static void sendMediaCommand(MediaCommandType type,
                             ElderlyPreference preference = PREF_NONE,
                             uint8_t selection_index = 0) {
  if (!hub_connected || !command_char) {
    Serial.println("Hub not connected — command queued locally only");
    return;
  }

  MediaCommand cmd{};
  cmd.timestamp_ms = millis();
  cmd.command = type;
  cmd.preference = preference;
  cmd.selection_index = selection_index;

  uint8_t payload[sizeof(MediaCommand)];
  memcpy(payload, &cmd, sizeof(cmd));
  command_char->writeValue(payload, sizeof(payload), false);
}

static void onMediaRecommendations(BLERemoteCharacteristic*,
                                   uint8_t* data, size_t length,
                                   bool) {
  if (length < sizeof(MediaRecommendationList)) {
    return;
  }

  MediaRecommendationList list{};
  memcpy(&list, data, sizeof(list));
  media_player.setRecommendations(list);
  hapticPulse(150);
}

static bool connectToHub(BLEAdvertisedDevice& device) {
  if (hub_client && hub_client->isConnected()) {
    return true;
  }

  if (!hub_client) {
    hub_client = BLEDevice::createClient();
  }

  if (!hub_client->connect(&device)) {
    Serial.println("Failed to connect to waist hub");
    return false;
  }

  BLERemoteService* service =
      hub_client->getService(WALKING_STICK_SERVICE_UUID);
  if (!service) {
    Serial.println("Hub service not found");
    hub_client->disconnect();
    return false;
  }

  command_char = service->getCharacteristic(COMMAND_CHAR_UUID);
  media_char = service->getCharacteristic(MEDIA_CHAR_UUID);
  if (!command_char || !media_char) {
    Serial.println("Hub media characteristics not found");
    hub_client->disconnect();
    return false;
  }

  if (media_char->canNotify()) {
    media_char->registerForNotify(onMediaRecommendations);
  }

  hub_connected = true;
  Serial.println("Connected to waist hub for media");
  return true;
}

class StickScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertised_device) override {
    if (!advertised_device.haveServiceUUID()) {
      return;
    }

    BLEUUID service_uuid(WALKING_STICK_SERVICE_UUID);
    if (!advertised_device.isAdvertisingService(service_uuid)) {
      return;
    }

    const std::string name = advertised_device.getName();
    if (name.find("Waist") == std::string::npos) {
      return;
    }

    Serial.printf("Waist hub detected: %s RSSI=%d\n", name.c_str(),
                  advertised_device.getRSSI());

    if (!hub_connected) {
      connectToHub(advertised_device);
    }
  }
};

static void triggerLocalAlert(AlertType type) {
  alert_active = true;
  digitalWrite(pins::stick::VIBRATOR, HIGH);
  media_player.pauseForSafety();

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
  media_player.resumeAfterSafety();
}

static void handleButtonEvent(const ButtonEvent& event) {
  hapticPulse(40);

  switch (event.button_id) {
    case BTN_PLAY:
      if (event.action == BUTTON_SHORT_PRESS) {
        media_player.togglePlayPause();
        sendMediaCommand(MEDIA_CMD_PLAY_PAUSE);
      }
      break;

    case BTN_NEXT:
      if (event.action == BUTTON_SHORT_PRESS) {
        media_player.nextTrack();
        sendMediaCommand(MEDIA_CMD_NEXT);
      } else {
        media_player.previousTrack();
        sendMediaCommand(MEDIA_CMD_PREVIOUS);
      }
      break;

    case BTN_VOLUME:
      if (event.action == BUTTON_SHORT_PRESS) {
        media_player.volumeUp();
        sendMediaCommand(MEDIA_CMD_VOLUME_UP);
      } else {
        media_player.volumeDown();
        sendMediaCommand(MEDIA_CMD_VOLUME_DOWN);
      }
      break;

    case BTN_RECOMMEND:
      if (event.action == BUTTON_SHORT_PRESS) {
        sendMediaCommand(MEDIA_CMD_REQUEST_RECOMMENDATIONS, active_preference);
      } else {
        active_preference = static_cast<ElderlyPreference>(
            (static_cast<uint8_t>(active_preference) % 5) + 1);
        Serial.printf("Preference set: %u\n", active_preference);
        sendMediaCommand(MEDIA_CMD_SET_PREFERENCE, active_preference);
        hapticPulse(100);
      }
      break;

    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(pins::stick::STATUS_LED, OUTPUT);
  pinMode(pins::stick::BUTTON_ALERT, INPUT_PULLUP);
  pinMode(pins::stick::VIBRATOR, OUTPUT);
  digitalWrite(pins::stick::VIBRATOR, LOW);

  const int media_buttons[] = {
      pins::stick::BUTTON_PLAY,
      pins::stick::BUTTON_NEXT,
      pins::stick::BUTTON_VOLUME,
      pins::stick::BUTTON_RECOMMEND,
  };
  input_handler.begin(media_buttons, 4);
  media_player.begin();

  BLEDevice::init(BLE_DEVICE_NAME);
  ble_scan = BLEDevice::getScan();
  ble_scan->setAdvertisedDeviceCallbacks(new StickScanCallbacks());
  ble_scan->setActiveScan(true);
  ble_scan->setInterval(100);
  ble_scan->setWindow(99);

  Serial.printf("[%s] walking stick ready (media player enabled)\n",
                deviceRoleName(DEVICE_WALKING_STICK));
  Serial.println("Buttons: PLAY | NEXT | VOL (short=up, long=down) | "
                 "RECOMMEND (short=request, long=set preference)");
}

void loop() {
  static uint32_t last_scan = 0;
  static uint32_t last_battery_check = 0;

  ButtonEvent button_event{};
  if (input_handler.poll(&button_event)) {
    handleButtonEvent(button_event);
  }

  if (digitalRead(pins::stick::BUTTON_ALERT) == LOW) {
    triggerLocalAlert(ALERT_TYPE_SOS);
    delay(300);
  }

  if (!hub_connected && !scan_in_progress &&
      millis() - last_scan >= config::BLE_SCAN_INTERVAL_MS) {
    last_scan = millis();
    scan_in_progress = true;
    ble_scan->start(3, false);
    scan_in_progress = false;
  }

  if (hub_client && hub_connected && !hub_client->isConnected()) {
    hub_connected = false;
    command_char = nullptr;
    media_char = nullptr;
    Serial.println("Lost connection to waist hub");
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

  const bool media_active = media_player.isPlaying();
  digitalWrite(pins::stick::STATUS_LED,
               alert_active ? HIGH
                            : (media_active ? HIGH
                                              : !digitalRead(pins::stick::STATUS_LED)));
  delay(10);
}
