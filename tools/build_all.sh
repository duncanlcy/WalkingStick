#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

TARGETS=(waist_hub walking_stick shoe_pad_left shoe_pad_right)

if ! command -v pio >/dev/null 2>&1; then
  echo "PlatformIO CLI (pio) is required. Install: https://platformio.org/install/cli"
  exit 1
fi

echo "Building all WalkingStick firmware targets..."
for target in "${TARGETS[@]}"; do
  echo "==> $target"
  pio run -e "$target"
done

echo "All targets built successfully."
