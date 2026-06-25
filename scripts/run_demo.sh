#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PORT="${PORT:-8080}"

cd "$ROOT"

if ! command -v python3 >/dev/null 2>&1; then
  if command -v python >/dev/null 2>&1; then
    PYTHON=python
  else
    echo "Python 3 is required to run the demo" >&2
    exit 1
  fi
else
  PYTHON=python3
fi

echo "Starting WalkingStick demo on http://localhost:${PORT}"
exec "$PYTHON" demo/server.py --port "$PORT" "$@"
