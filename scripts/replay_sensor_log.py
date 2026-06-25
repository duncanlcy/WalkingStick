#!/usr/bin/env python3
"""Replay recorded or synthetic sensor data through the gait analysis pipeline."""

import argparse
import json
import sys
import time
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from backend.alerts.engine import AlertEngine
from backend.sensors.models import SensorReading


def load_readings(path: Path) -> list[SensorReading]:
    with open(path) as f:
        data = json.load(f)
    items = data if isinstance(data, list) else data.get("readings", [])
    return [SensorReading.from_dict(r) for r in items]


def fetch_sample(behavior: str, base_url: str) -> list[SensorReading]:
    url = f"{base_url}/api/sample/{behavior}"
    with urllib.request.urlopen(url) as resp:
        data = json.loads(resp.read())
    return [SensorReading.from_dict(r) for r in data["readings"]]


def replay(readings: list[SensorReading], realtime: bool = False) -> None:
    engine = AlertEngine()
    alert_count = 0

    print(f"Replaying {len(readings)} sensor readings...\n")

    for i, reading in enumerate(readings):
        result, alert = engine.process_reading(reading)

        if realtime and i > 0:
            dt = reading.timestamp - readings[i - 1].timestamp
            time.sleep(max(0, dt))

        if result and result.is_unstable:
            print(f"  [{reading.timestamp:.2f}] UNSTABLE score={result.features.stability_score:.1f} severity={result.overall_severity.value}")

        if alert:
            alert_count += 1
            print(f"  *** ALERT [{alert.severity.value}] {alert.message}")
            if alert.voice_message:
                print(f"      Voice: \"{alert.voice_message}\"")

    print(f"\nDone. {alert_count} alert(s) emitted.")


def main():
    parser = argparse.ArgumentParser(description="Replay sensor data through gait analyzer")
    parser.add_argument("--file", type=Path, help="JSON file with sensor readings")
    parser.add_argument("--behavior", choices=["stable", "unstable", "near_fall"], default="unstable")
    parser.add_argument("--server", default="http://localhost:8000", help="API server for sample data")
    parser.add_argument("--realtime", action="store_true", help="Replay at original timing")
    args = parser.parse_args()

    if args.file:
        readings = load_readings(args.file)
    else:
        readings = fetch_sample(args.behavior, args.server)

    replay(readings, realtime=args.realtime)


if __name__ == "__main__":
    main()
