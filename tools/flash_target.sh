#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

TARGET="${1:-}"

if ! command -v pio >/dev/null 2>&1; then
  echo "PlatformIO CLI (pio) is required. Install: https://platformio.org/install/cli"
  exit 1
fi

if [[ -z "$TARGET" ]]; then
  echo "Usage: $0 <waist_hub|walking_stick|shoe_pad_left|shoe_pad_right>"
  exit 1
fi

pio run -e "$TARGET" -t upload
