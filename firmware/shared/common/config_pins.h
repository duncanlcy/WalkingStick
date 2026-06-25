#pragma once

// Pin maps are selected per firmware target via compile-time role flags.

#if defined(ROLE_WAIST_HUB)

// ESP32 waist safety pad — data collector and alert hub
constexpr int kStatusLedPin = 2;
constexpr int kBuzzerPin = 25;
constexpr int kSdCsPin = 5;
constexpr int kImuSdaPin = 21;
constexpr int kImuSclPin = 22;
constexpr int kBatteryAdcPin = 34;

#elif defined(ROLE_WALKING_STICK)

// ESP32-C3 walking stick — balance and tilt sensing
constexpr int kStatusLedPin = 8;
constexpr int kImuSdaPin = 4;
constexpr int kImuSclPin = 5;
constexpr int kLoadCellDoutPin = 6;
constexpr int kLoadCellSckPin = 7;
constexpr int kBatteryAdcPin = 0;

#elif defined(ROLE_SHOE_PAD)

// ESP32-C3 shoe insole pad — gait pressure sensing
constexpr int kStatusLedPin = 8;
constexpr int kFsHeelPin = 0;
constexpr int kFsMidfootPin = 1;
constexpr int kFsForefootPin = 2;
constexpr int kBatteryAdcPin = 3;

#endif
