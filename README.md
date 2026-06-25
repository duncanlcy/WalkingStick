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

## Localhost demo

A browser-based simulator mirrors the three-device architecture, safety thresholds,
and BLE data flow without hardware.

### Windows (`E:\WalkingStick`)

**PowerShell** (recommended):

```powershell
cd E:\WalkingStick
.\scripts\run_demo.ps1
```

**Command Prompt**:

```cmd
cd E:\WalkingStick
scripts\run_demo.bat
```

You should see:

```
============================================================
WalkingStick demo is running
Open in your browser: http://localhost:8080
Press Ctrl+C to stop.
============================================================
```

Then open **http://localhost:8080** if the browser does not open automatically.

> Do **not** double-click `demo/index.html` — the page must be served over
> `http://localhost` or the simulator will not load.

### macOS / Linux

```bash
./scripts/run_demo.sh
```

Use the scenario buttons to trigger normal gait, imbalance, fall, impact, SOS, and
low-battery events. Telemetry updates every 50 ms using the same thresholds as
`include/config.h` and `include/safety.h`.

### Troubleshooting

| Problem | Fix |
|---------|-----|
| `demo` folder missing | Run `git pull` in the repo root |
| `python` not found | Install Python 3 and check "Add to PATH" on Windows |
| Port 8080 in use | `python demo/server.py --port 8081` |
| Blank page | Use `http://localhost:8080`, not `file:///.../index.html` |

## CI

GitHub Actions builds all three environments on every push and pull request.

## Next steps

1. Wire sensors per `docs/hardware.md`
2. Calibrate FSR pressure thresholds on shoe pads
3. Tune fall-detection thresholds on the waist pad
4. Add SD card logging driver on the waist safety pad (SPI pins reserved in config)
