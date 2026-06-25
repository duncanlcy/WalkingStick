# WalkingStick

A smart walking aid monitoring system that detects **unstable walking behaviors** from sensor data and issues **visual and voice warnings** in real time.

## How It Works

```
Sensor Data (IMU) → Gait Analyzer → Alert Engine → Visual + Voice Warnings
```

1. **Sensor input** — Accelerometer and gyroscope readings (from a walking stick, wearable, or phone IMU)
2. **Gait analysis** — Extracts step variability, lateral sway, cadence, asymmetry, and near-fall signatures
3. **Alert engine** — Applies thresholds, debouncing, and cooldowns to avoid alarm fatigue
4. **Output** — Full-screen visual overlays and spoken warnings via the Web Speech API

## Quick Start

```bash
# Install dependencies
pip install -r requirements.txt

# Start the server
uvicorn backend.main:app --reload --host 0.0.0.0 --port 8000

# Open the dashboard
# http://localhost:8000
```

1. Select a walking behavior (stable, unstable, or near-fall)
2. Click **Start Monitoring**
3. Watch the stability score and flags update in real time
4. Visual and voice warnings fire when instability is detected

## Project Structure

```
WalkingStick/
├── backend/
│   ├── main.py              # FastAPI server + WebSocket streaming
│   ├── gait/
│   │   ├── analyzer.py      # Feature extraction & instability detection
│   │   └── types.py         # Data models
│   ├── alerts/
│   │   └── engine.py        # Alert policy, debouncing, voice message generation
│   └── sensors/
│       └── models.py        # Sensor reading models
├── frontend/
│   ├── index.html           # Dashboard UI
│   ├── css/style.css        # Visual alert styling
│   └── js/
│       ├── app.js           # Main app wiring
│       ├── alerts.js        # Visual overlay + voice (TTS)
│       └── sensor-sim.js    # Simulated IMU data for demo
├── shared/
│   └── gait_thresholds.json # Tunable instability thresholds & messages
├── scripts/
│   └── replay_sensor_log.py # CLI replay tool for testing
└── data/
    └── sample_unstable_walk.json
```

## API

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Web dashboard |
| `/api/health` | GET | Health check |
| `/api/analyze` | POST | Batch analyze sensor readings |
| `/api/sample/{behavior}` | GET | Get synthetic sensor data |
| `/ws/{session_id}` | WebSocket | Real-time sensor streaming |

### WebSocket Protocol

Send sensor readings:
```json
{ "type": "sensor", "reading": { "timestamp": 1.0, "accel_x": 0.1, "accel_y": 0.05, "accel_z": 9.8, "gyro_x": 0, "gyro_y": 0, "gyro_z": 0 } }
```

Receive analysis + alerts:
```json
{ "type": "update", "analysis": { "stability_score": 62, "overall_severity": "caution", ... }, "alert": { "severity": "caution", "message": "...", "voice_message": "Caution. ..." } }
```

## Instability Metrics

| Metric | What it detects |
|--------|----------------|
| Step time variability | Irregular, uneven steps |
| Lateral sway | Side-to-side balance loss |
| Cadence drop | Sudden slowing |
| Step asymmetry | Uneven left/right steps |
| Pitch/roll variance | Near-fall balance shifts |
| Impact spike | Sudden deceleration or stumble |

## Alert Severities

| Level | Visual | Voice |
|-------|--------|-------|
| **Watch** | Yellow overlay, 4s | Quiet spoken notice |
| **Caution** | Orange pulsing overlay, 6s | "Caution. …" |
| **Critical** | Red pulsing overlay, 10s | "Warning! …" (loudest) |

## Replay Tool

Test with synthetic or recorded data from the command line:

```bash
# Replay unstable walking (fetches from running server)
python scripts/replay_sensor_log.py --behavior unstable

# Replay a saved JSON log
python scripts/replay_sensor_log.py --file data/sample_unstable_walk.json
```

## Configuration

Edit `shared/gait_thresholds.json` to tune detection sensitivity, cooldowns, and warning messages without changing code.

## License

MIT
