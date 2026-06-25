#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if ! command -v pio >/dev/null 2>&1; then
  echo "PlatformIO CLI (pio) is required. Install: https://platformio.org/install/cli"
  exit 1
fi

TARGETS=("$@")
if [ ${#TARGETS[@]} -eq 0 ]; then
  TARGETS=(waist_safety_pad shoe_pad walking_stick)
fi

echo "Building firmware targets: ${TARGETS[*]}"
for env in "${TARGETS[@]}"; do
  echo "==> Building $env"
  pio run -e "$env"
done

echo "All builds succeeded."
