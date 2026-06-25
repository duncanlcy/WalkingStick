#pragma once

#include <Arduino.h>
#include "config.h"
#include "protocol.h"

// Podcast and music player for the walking stick handle.
// Uses I2S when a speaker is connected; logs playback to serial otherwise.
class MediaPlayer {
 public:
  void begin() {
    volume_ = config::MEDIA_DEFAULT_VOLUME;
    state_ = MEDIA_STATE_STOPPED;
    content_type_ = MEDIA_NONE;
    now_playing_[0] = '\0';
    now_playing_source_[0] = '\0';
    recommendations_.count = 0;
    recommendations_.greeting[0] = '\0';
    paused_by_safety_ = false;

    pinMode(pins::stick::I2S_BCK, OUTPUT);
    pinMode(pins::stick::I2S_LRCK, OUTPUT);
    pinMode(pins::stick::I2S_DOUT, OUTPUT);
    digitalWrite(pins::stick::I2S_BCK, LOW);
    digitalWrite(pins::stick::I2S_LRCK, LOW);
    digitalWrite(pins::stick::I2S_DOUT, LOW);
  }

  void setRecommendations(const MediaRecommendationList& list) {
    recommendations_ = list;
    state_ = MEDIA_STATE_BROWSING;
    Serial.printf("Recommendations: %s\n", list.greeting);
    for (uint8_t i = 0; i < list.count; ++i) {
      Serial.printf("  [%u] %s — %s (%s)\n", i + 1, list.items[i].title,
                    list.items[i].subtitle,
                    list.items[i].content_type == MEDIA_PODCAST ? "podcast" : "music");
    }
  }

  void playRecommendation(uint8_t index) {
    if (index >= recommendations_.count) {
      return;
    }

    const MediaItem& item = recommendations_.items[index];
    content_type_ = item.content_type;
    strncpy(now_playing_, item.title, sizeof(now_playing_) - 1);
    strncpy(now_playing_source_, item.subtitle, sizeof(now_playing_source_) - 1);
    state_ = MEDIA_STATE_PLAYING;
    paused_by_safety_ = false;

    Serial.printf("Now playing: %s — %s\n", now_playing_, now_playing_source_);
  }

  void togglePlayPause() {
    if (state_ == MEDIA_STATE_PLAYING) {
      state_ = MEDIA_STATE_PAUSED;
      Serial.println("Playback paused");
      return;
    }

    if (state_ == MEDIA_STATE_PAUSED) {
      state_ = MEDIA_STATE_PLAYING;
      Serial.println("Playback resumed");
      return;
    }

    if (recommendations_.count > 0) {
      playRecommendation(0);
    }
  }

  void nextTrack() {
    if (recommendations_.count == 0) {
      return;
    }

    uint8_t current = 0;
    for (uint8_t i = 0; i < recommendations_.count; ++i) {
      if (strncmp(recommendations_.items[i].title, now_playing_,
                  sizeof(now_playing_)) == 0) {
        current = i;
        break;
      }
    }

    playRecommendation((current + 1) % recommendations_.count);
  }

  void previousTrack() {
    if (recommendations_.count == 0) {
      return;
    }

    uint8_t current = 0;
    for (uint8_t i = 0; i < recommendations_.count; ++i) {
      if (strncmp(recommendations_.items[i].title, now_playing_,
                  sizeof(now_playing_)) == 0) {
        current = i;
        break;
      }
    }

    const uint8_t prev =
        current == 0 ? recommendations_.count - 1 : current - 1;
    playRecommendation(prev);
  }

  void volumeUp() {
    if (volume_ + config::MEDIA_VOLUME_STEP <= config::MEDIA_MAX_VOLUME) {
      volume_ += config::MEDIA_VOLUME_STEP;
      Serial.printf("Volume: %u%%\n", volume_);
    }
  }

  void volumeDown() {
    if (volume_ >= config::MEDIA_VOLUME_STEP) {
      volume_ -= config::MEDIA_VOLUME_STEP;
      Serial.printf("Volume: %u%%\n", volume_);
    }
  }

  void pauseForSafety() {
    if (state_ == MEDIA_STATE_PLAYING) {
      state_ = MEDIA_STATE_PAUSED;
      paused_by_safety_ = true;
      Serial.println("Playback paused for safety alert");
    }
  }

  void resumeAfterSafety() {
    if (paused_by_safety_ && state_ == MEDIA_STATE_PAUSED) {
      state_ = MEDIA_STATE_PLAYING;
      paused_by_safety_ = false;
      Serial.println("Playback resumed after safety alert");
    }
  }

  MediaStatus status() const {
    MediaStatus s{};
    s.timestamp_ms = millis();
    s.state = state_;
    s.content_type = content_type_;
    s.volume_percent = volume_;
    strncpy(s.now_playing, now_playing_, sizeof(s.now_playing));
    strncpy(s.now_playing_source, now_playing_source_,
            sizeof(s.now_playing_source));
    s.has_recommendations = recommendations_.count > 0;
    return s;
  }

  bool isPlaying() const { return state_ == MEDIA_STATE_PLAYING; }

 private:
  uint8_t volume_ = config::MEDIA_DEFAULT_VOLUME;
  MediaPlayerState state_ = MEDIA_STATE_STOPPED;
  MediaContentType content_type_ = MEDIA_NONE;
  char now_playing_[48]{};
  char now_playing_source_[32]{};
  MediaRecommendationList recommendations_{};
  bool paused_by_safety_ = false;
};
