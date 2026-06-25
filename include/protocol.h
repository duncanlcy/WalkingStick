#pragma once

#include <Arduino.h>
#include "device_roles.h"

// BLE service UUIDs — shared across all nodes
#define WALKING_STICK_SERVICE_UUID "a1b2c3d4-e5f6-7890-abcd-ef1234567890"
#define TELEMETRY_CHAR_UUID "a1b2c3d4-e5f6-7890-abcd-ef1234567891"
#define COMMAND_CHAR_UUID "a1b2c3d4-e5f6-7890-abcd-ef1234567892"
#define ALERT_CHAR_UUID "a1b2c3d4-e5f6-7890-abcd-ef1234567893"
#define MEDIA_CHAR_UUID "a1b2c3d4-e5f6-7890-abcd-ef1234567894"

enum AlertLevel : uint8_t {
  ALERT_NONE = 0,
  ALERT_INFO = 1,
  ALERT_WARNING = 2,
  ALERT_CRITICAL = 3,
};

enum AlertType : uint8_t {
  ALERT_TYPE_NONE = 0,
  ALERT_TYPE_FALL_DETECTED = 1,
  ALERT_TYPE_IMPACT = 2,
  ALERT_TYPE_GAIT_IRREGULAR = 3,
  ALERT_TYPE_LOW_BATTERY = 4,
  ALERT_TYPE_SOS = 5,
};

struct SensorSample {
  uint32_t timestamp_ms;
  float accel_x;
  float accel_y;
  float accel_z;
  uint16_t pressure_left;
  uint16_t pressure_right;
  uint8_t battery_percent;
  DeviceRole source;
};

struct AlertEvent {
  uint32_t timestamp_ms;
  AlertLevel level;
  AlertType type;
  DeviceRole source;
  char message[64];
};

struct TelemetryPacket {
  uint8_t protocol_version;
  DeviceRole source;
  SensorSample sample;
  AlertEvent alert;
  bool has_alert;
};

enum MediaContentType : uint8_t {
  MEDIA_NONE = 0,
  MEDIA_MUSIC = 1,
  MEDIA_PODCAST = 2,
};

enum MediaPlayerState : uint8_t {
  MEDIA_STATE_STOPPED = 0,
  MEDIA_STATE_PLAYING = 1,
  MEDIA_STATE_PAUSED = 2,
  MEDIA_STATE_BROWSING = 3,
};

enum MediaCommandType : uint8_t {
  MEDIA_CMD_NONE = 0,
  MEDIA_CMD_PLAY_PAUSE = 1,
  MEDIA_CMD_NEXT = 2,
  MEDIA_CMD_PREVIOUS = 3,
  MEDIA_CMD_VOLUME_UP = 4,
  MEDIA_CMD_VOLUME_DOWN = 5,
  MEDIA_CMD_REQUEST_RECOMMENDATIONS = 6,
  MEDIA_CMD_SELECT_RECOMMENDATION = 7,
  MEDIA_CMD_SET_PREFERENCE = 8,
};

// Elderly-friendly content preferences (selected via long-press on recommend button)
enum ElderlyPreference : uint8_t {
  PREF_NONE = 0,
  PREF_CALM = 1,
  PREF_ENERGETIC = 2,
  PREF_NEWS = 3,
  PREF_STORIES = 4,
  PREF_CLASSICS = 5,
};

struct MediaCommand {
  uint32_t timestamp_ms;
  MediaCommandType command;
  ElderlyPreference preference;
  uint8_t selection_index;
};

struct MediaItem {
  char title[48];
  char subtitle[32];
  MediaContentType content_type;
  char stream_url[96];
};

struct MediaRecommendationList {
  uint8_t count;
  MediaItem items[5];
  char greeting[64];
};

struct MediaStatus {
  uint32_t timestamp_ms;
  MediaPlayerState state;
  MediaContentType content_type;
  uint8_t volume_percent;
  char now_playing[48];
  char now_playing_source[32];
  bool has_recommendations;
};
