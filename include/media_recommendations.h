#pragma once

#include <Arduino.h>
#include "config.h"
#include "protocol.h"

// Rule-based music and podcast recommendations tuned for elderly wearers.
class MediaRecommendationEngine {
 public:
  void setPreference(ElderlyPreference pref) {
    if (pref != PREF_NONE) {
      preference_ = pref;
    }
  }

  ElderlyPreference preference() const { return preference_; }

  ElderlyPreference cyclePreference() {
    switch (preference_) {
      case PREF_NONE:
      case PREF_CLASSICS:
        preference_ = PREF_CALM;
        break;
      case PREF_CALM:
        preference_ = PREF_ENERGETIC;
        break;
      case PREF_ENERGETIC:
        preference_ = PREF_NEWS;
        break;
      case PREF_NEWS:
        preference_ = PREF_STORIES;
        break;
      case PREF_STORIES:
        preference_ = PREF_CLASSICS;
        break;
    }
    return preference_;
  }

  MediaRecommendationList recommend(uint32_t hour_of_day,
                                  bool gait_irregular) const {
    MediaRecommendationList list{};
    list.count = 0;

    if (gait_irregular) {
      buildGreeting(list, hour_of_day, true);
      addItem(list, "Gentle Piano Walk", "Calm Classics", MEDIA_MUSIC,
              "https://example.com/music/gentle-piano");
      addItem(list, "Mindful Moments", "Wellness Podcast", MEDIA_PODCAST,
              "https://example.com/podcast/mindful-moments");
      addItem(list, "Soft Jazz Afternoon", "Easy Listening", MEDIA_MUSIC,
              "https://example.com/music/soft-jazz");
      return list;
    }

    switch (preference_) {
      case PREF_CALM:
        buildGreeting(list, hour_of_day, false);
        addItem(list, "Morning Birdsong", "Nature Sounds", MEDIA_MUSIC,
                "https://example.com/music/birdsong");
        addItem(list, "Tea Time Tales", "Cozy Stories", MEDIA_PODCAST,
                "https://example.com/podcast/tea-time");
        addItem(list, "Slow Strings", "Calm Classics", MEDIA_MUSIC,
                "https://example.com/music/slow-strings");
        break;

      case PREF_ENERGETIC:
        buildGreeting(list, hour_of_day, false);
        addItem(list, "Swingin' Standards", "Big Band Hits", MEDIA_MUSIC,
                "https://example.com/music/swing");
        addItem(list, "Walk With Me", "Fitness Podcast", MEDIA_PODCAST,
                "https://example.com/podcast/walk-with-me");
        addItem(list, "Upbeat Oldies", "Golden Era", MEDIA_MUSIC,
                "https://example.com/music/oldies");
        break;

      case PREF_NEWS:
        buildGreeting(list, hour_of_day, false);
        addItem(list, "Morning Headlines", "Daily News", MEDIA_PODCAST,
                "https://example.com/podcast/headlines");
        addItem(list, "Community Bulletin", "Local News", MEDIA_PODCAST,
                "https://example.com/podcast/community");
        addItem(list, "World in Five", "Global Briefing", MEDIA_PODCAST,
                "https://example.com/podcast/world-five");
        break;

      case PREF_STORIES:
        buildGreeting(list, hour_of_day, false);
        addItem(list, "Grandma's Tales", "Story Hour", MEDIA_PODCAST,
                "https://example.com/podcast/grandmas-tales");
        addItem(list, "Classic Radio Drama", "Retro Theatre", MEDIA_PODCAST,
                "https://example.com/podcast/radio-drama");
        addItem(list, "Poetry for the Path", "Spoken Word", MEDIA_PODCAST,
                "https://example.com/podcast/poetry");
        break;

      case PREF_CLASSICS:
      case PREF_NONE:
      default:
        buildGreeting(list, hour_of_day, false);
        addItem(list, "Moonlight Serenade", "Glenn Miller", MEDIA_MUSIC,
                "https://example.com/music/moonlight");
        addItem(list, "What a Wonderful World", "Louis Armstrong", MEDIA_MUSIC,
                "https://example.com/music/wonderful-world");
        addItem(list, "Memory Lane", "Nostalgia Podcast", MEDIA_PODCAST,
                "https://example.com/podcast/memory-lane");
        break;
    }

    return list;
  }

  const char* preferenceLabel(ElderlyPreference pref) const {
    switch (pref) {
      case PREF_CALM: return "calm music and stories";
      case PREF_ENERGETIC: return "upbeat music";
      case PREF_NEWS: return "news podcasts";
      case PREF_STORIES: return "story podcasts";
      case PREF_CLASSICS: return "classic favorites";
      default: return "general favorites";
    }
  }

 private:
  ElderlyPreference preference_ = PREF_NONE;

  static void addItem(MediaRecommendationList& list, const char* title,
                      const char* subtitle, MediaContentType type,
                      const char* url) {
    if (list.count >= config::MEDIA_MAX_RECOMMENDATIONS) {
      return;
    }

    MediaItem& item = list.items[list.count++];
    strncpy(item.title, title, sizeof(item.title) - 1);
    strncpy(item.subtitle, subtitle, sizeof(item.subtitle) - 1);
    item.content_type = type;
    strncpy(item.stream_url, url, sizeof(item.stream_url) - 1);
  }

  void buildGreeting(MediaRecommendationList& list, uint32_t hour,
                     bool calming) const {
    if (calming) {
      strncpy(list.greeting,
              "Take it easy — here are some calming picks for your walk.",
              sizeof(list.greeting) - 1);
      return;
    }

    if (hour < 12) {
      snprintf(list.greeting, sizeof(list.greeting),
               "Good morning! Here are %s for you.",
               preferenceLabel(preference_));
    } else if (hour < 17) {
      snprintf(list.greeting, sizeof(list.greeting),
               "Good afternoon! Here are %s for your stroll.",
               preferenceLabel(preference_));
    } else {
      snprintf(list.greeting, sizeof(list.greeting),
               "Good evening! Here are %s to unwind.",
               preferenceLabel(preference_));
    }
  }
};
