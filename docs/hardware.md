# Hardware Bill of Materials

## Waist Safety Pad (Data Collector + Protection)

| Part | Qty | Notes |
|------|-----|-------|
| ESP32 DevKit | 1 | Main hub, BLE + logging |
| MPU6050 or BMI270 | 1 | Torso orientation / fall detection |
| Piezo buzzer | 1 | Alert feedback |
| 3.7 V LiPo + charger | 1 | Wearable power |
| Padded waist belt | 1 | Mounting and impact cushioning |
| Status LED | 1 | Connection indicator |

**GPIO map** — see `firmware/shared/common/config_pins.h` (`ROLE_WAIST_HUB`).

## Walking Stick

| Part | Qty | Notes |
|------|-----|-------|
| ESP32-C3 DevKit | 1 | Low-power BLE peripheral |
| MPU6050 or BMI270 | 1 | Stick tilt and swing |
| Load cell + HX711 (optional) | 1 | Weight on stick |
| 3.7 V LiPo | 1 | Internal battery |
| Hollow walking stick tube | 1 | Enclosure |

## Shoe Pads (Left + Right)

| Part | Qty | Notes |
|------|-----|-------|
| ESP32-C3 DevKit | 2 | One per foot |
| FSR pressure sensors | 6 | Heel, midfoot, forefoot × 2 shoes |
| Thin insole material | 2 | Sensor mounting |
| 3.7 V LiPo (flat) | 2 | One per pad |

Build **left** and **right** firmware separately:

```bash
pio run -e shoe_pad_left -t upload
pio run -e shoe_pad_right -t upload
```

## Wiring Notes

- Use I2C for IMU modules (SDA/SCL per pin map).
- FSR sensors need a 10 kΩ divider to GND on each analog input.
- Keep BLE antennas unobstructed inside enclosures.
- Add a power switch on each module for field servicing.

## Enclosure Checklist

- [ ] Waist pad: padded compartment for ESP32 and battery
- [ ] Stick: USB access for flashing without disassembly
- [ ] Shoe pads: waterproof layer over FSR sensors
- [ ] Strain relief on all flex wires entering shoes
