# WalkingStick Platform

Firmware and build tooling for a distributed mobility-assist system:

| Device | Role |
|--------|------|
| **Walking stick** | Handle UI, haptic alerts, BLE scanning, SOS button |
| **Shoe pad** | Foot-pressure sensors for gait and balance monitoring |
| **Waist safety pad** | Fall/impact detection, protection zone, central data collector |

All three nodes communicate over BLE using a shared protocol defined in `src/common/`.

## Repository layout

```
WalkingStick/
├── platformio.ini          # Build environments for all three devices
├── include/                # Shared headers (config, protocol, sensors, safety)
├── src/
│   ├── waist_safety_pad/   # Hub firmware
│   ├── shoe_pad/           # Insole sensor firmware
│   └── walking_stick/      # Handle firmware
├── scripts/
│   ├── build_all.sh        # Build every target
│   └── flash.sh            # Flash a single target
└── docs/
    ├── architecture.md
    └── hardware.md
```

## Prerequisites

- [PlatformIO CLI](https://platformio.org/install/cli) or the PlatformIO IDE extension
- ESP32 dev boards (one per device node)
- USB cable for flashing

## Build

Build all firmware targets:

```bash
./scripts/build_all.sh
```

Build a single target:

```bash
pio run -e waist_safety_pad
pio run -e shoe_pad
pio run -e walking_stick
```

## Flash

Connect the target board, then:

```bash
./scripts/flash.sh waist_safety_pad   # or shoe_pad / walking_stick
```

## Hardware overview

See [docs/hardware.md](docs/hardware.md) for BOM, wiring, and pin assignments.

See [docs/architecture.md](docs/architecture.md) for system design and data flow.

## Configuration

Edit `include/config.h` to adjust:

- GPIO pin assignments per PCB revision
- Fall/impact acceleration thresholds
- Gait imbalance sensitivity
- Sample and telemetry intervals

## CI

GitHub Actions builds all three environments on every push and pull request.

## Next steps

1. Wire sensors per `docs/hardware.md`
2. Calibrate FSR pressure thresholds on shoe pads
3. Tune fall-detection thresholds on the waist pad
4. Add SD card logging driver on the waist safety pad (SPI pins reserved in config)
