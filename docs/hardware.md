# Hardware Guide

## Bill of materials (per node)

### Waist safety pad (hub)

| Component | Qty | Notes |
|-----------|-----|-------|
| ESP32 dev board | 1 | 3.3 V logic |
| MPU6050 or BMI160 IMU | 1 | I2C, waist-mounted |
| Piezo buzzer | 1 | Alert output |
| Status LED | 1 | GPIO 2 |
| microSD module (optional) | 1 | SPI, CS on GPIO 5 |
| LiPo 3.7 V + charger | 1 | 500–1000 mAh |
| Padded waist belt enclosure | 1 | Impact protection |

### Shoe pad (sensor)

| Component | Qty | Notes |
|-----------|-----|-------|
| ESP32 dev board (or ESP32-C3 mini) | 1 | Fits insole cavity |
| FSR 402 or 406 | 4 | Heel + toe, left + right |
| 10 kΩ resistors | 4 | FSR voltage dividers |
| Thin flexible PCB or fabric pad | 1 | Sensor mounting |
| LiPo 3.7 V (flat) | 1 | 150–300 mAh |

### Walking stick (handle)

| Component | Qty | Notes |
|-----------|-----|-------|
| ESP32 dev board | 1 | Mounted in handle |
| Tactile push button | 1 | SOS, GPIO 0 |
| Tactile push buttons | 4 | Play, next, volume, recommend (GPIO 12–15) |
| I2S amplifier (MAX98357A) | 1 | Speaker output for music/podcasts |
| Small speaker (8 Ω) | 1 | 0.5–1 W, handle-mounted |
| Vibration motor + driver | 1 | Haptic feedback |
| Status LED | 1 | GPIO 2 |
| Voltage divider | 1 | Battery ADC on GPIO 36 |
| LiPo 3.7 V | 1 | 1000+ mAh |

## Pin assignments

Defaults are in `include/config.h`. Adjust when your PCB revision differs.

### Waist safety pad

| Signal | GPIO |
|--------|------|
| IMU interrupt | 4 |
| Buzzer | 25 |
| Status LED | 2 |
| SD card CS | 5 |

### Shoe pad

| Signal | GPIO |
|--------|------|
| FSR left heel | 34 |
| FSR left toe | 35 |
| FSR right heel | 32 |
| FSR right toe | 33 |
| Status LED | 2 |

### Walking stick

| Signal | GPIO |
|--------|------|
| SOS button | 0 (pull-up) |
| Play / pause button | 12 (pull-up) |
| Next track button | 13 (pull-up) |
| Volume button | 14 (pull-up, short=up, long=down) |
| Recommend button | 15 (pull-up, short=request, long=preference) |
| Vibrator | 26 |
| Status LED | 2 |
| Battery ADC | 36 |
| I2S BCK | 27 |
| I2S LRCK | 32 |
| I2S DOUT | 33 |

## Wiring notes

### FSR pressure circuit

Each FSR forms a voltage divider with a 10 kΩ resistor:

```
3.3V ── FSR ── ADC pin ── 10kΩ ── GND
```

Calibrate `MIN_PRESSURE_THRESHOLD` in `config.h` after assembly.

### IMU (waist pad)

Connect MPU6050 via I2C:

| IMU | ESP32 |
|-----|-------|
| SDA | 21 |
| SCL | 22 |
| INT | 4 |
| VCC | 3.3 V |
| GND | GND |

Replace the placeholder `AccelerometerSensor::read()` in `include/sensors.h` with your IMU driver.

### I2S speaker (walking stick)

Connect a MAX98357A I2S amplifier for podcast and music playback:

| MAX98357A | ESP32 |
|-----------|-------|
| BCLK | 27 |
| LRC | 32 |
| DIN | 33 |
| VIN | 3.3–5 V |
| GND | GND |

Place the speaker behind a grille in the handle. Default volume is capped at 70% for elderly-friendly listening.

## Assembly order

1. Flash **waist safety pad** firmware and verify serial output.
2. Flash **shoe pad** firmware; confirm pressure readings over serial.
3. Flash **walking stick** firmware; test SOS button, media buttons, and vibration.
4. Power all nodes; confirm BLE advertising names:
   - `WalkingStick-Waist`
   - `WalkingStick-Shoe`
   - `WalkingStick-Handle`

## Enclosure recommendations

- **Waist pad** — padded neoprene belt with rigid IMU mount; keep antenna area clear.
- **Shoe pad** — waterproof flexible insole pocket; route wires away from heel strike zone.
- **Walking stick** — IP54 handle compartment; place button where thumb rests naturally.
