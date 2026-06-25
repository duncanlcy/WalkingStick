# WalkingStick

A build-ready embedded firmware repository for a mobility assistance system with three hardware roles:

- **Walking stick** — balance and tilt sensing (ESP32-C3)
- **Shoe pads** — left/right insole pressure sensors (ESP32-C3)
- **Waist safety pad** — protection belt, BLE hub, data logging, and alerts (ESP32)

## Quick Start

### Prerequisites

- [PlatformIO CLI](https://platformio.org/install/cli) (`pio` command)
- USB cable for flashing each module

### Build all firmware

```bash
chmod +x tools/build_all.sh
./tools/build_all.sh
```

### Build a single target

```bash
pio run -e waist_hub
pio run -e walking_stick
pio run -e shoe_pad_left
pio run -e shoe_pad_right
```

### Flash to hardware

Connect the target board, then:

```bash
chmod +x tools/flash_target.sh
./tools/flash_target.sh waist_hub
```

## Repository Layout

```
WalkingStick/
├── platformio.ini          # Multi-target build configuration
├── firmware/
│   ├── main.cpp            # Role dispatcher
│   ├── waist_hub/          # Safety pad / data collector
│   ├── walking_stick/      # Stick sensor firmware
│   ├── shoe_pad/           # Insole pressure firmware
│   └── shared/             # Protocol, pins, sensor helpers
├── docs/
│   ├── architecture.md
│   └── hardware.md         # BOM and wiring
└── tools/
    ├── build_all.sh
    └── flash_target.sh
```

## Hardware Targets

| PlatformIO env | Hardware | Purpose |
|----------------|----------|---------|
| `waist_hub` | Waist safety pad | Collects BLE telemetry, logs to SPIFFS, triggers buzzer on alerts |
| `walking_stick` | Walking stick | Publishes IMU samples over BLE |
| `shoe_pad_left` | Left shoe pad | Publishes heel/midfoot/forefoot pressure |
| `shoe_pad_right` | Right shoe pad | Same as left, different BLE device name |

See [docs/hardware.md](docs/hardware.md) for the full bill of materials and pin maps.

## Protocol

Devices communicate using a shared binary packet format and BLE GATT service defined in `firmware/shared/protocol/`. The waist hub logs received samples to `/telemetry.csv` on SPIFFS.

Details: [docs/architecture.md](docs/architecture.md)

## Development Notes

- IMU reads are stubbed until physical sensors are wired; replace logic in `firmware/shared/common/imu_sensor.cpp`.
- Pin assignments live in `firmware/shared/common/config_pins.h` — adjust for your PCB layout.
- Serial monitor: `pio device monitor -b 115200`

## License

MIT
