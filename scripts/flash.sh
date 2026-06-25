#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

ENV="${1:-waist_safety_pad}"

if ! command -v pio >/dev/null 2>&1; then
  echo "PlatformIO CLI (pio) is required."
  exit 1
fi

pio run -e "$ENV" -t upload
