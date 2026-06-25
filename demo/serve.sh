#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-8080}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "WalkingStick design demo → http://localhost:${PORT}"
echo "Press Ctrl+C to stop."
cd "$ROOT"
python3 -m http.server "$PORT" --bind 127.0.0.1
