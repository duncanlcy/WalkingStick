#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PORT="${PORT:-8080}"

cd "$ROOT"

if ! command -v python3 >/dev/null 2>&1; then
  echo "python3 is required to run the demo" >&2
  exit 1
fi

echo "Starting WalkingStick demo on http://localhost:${PORT}"
exec python3 demo/server.py --port "$PORT" "$@"
