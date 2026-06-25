#!/usr/bin/env python3
"""Serve the WalkingStick localhost demo."""

from __future__ import annotations

import argparse
import functools
import http.server
import os
import socketserver
import sys
import webbrowser
from pathlib import Path

DEMO_DIR = Path(__file__).resolve().parent
DEFAULT_PORT = 8080


class ReusableTCPServer(socketserver.TCPServer):
  allow_reuse_address = True


class DemoHandler(http.server.SimpleHTTPRequestHandler):
  def __init__(self, *args, directory: str | None = None, **kwargs):
    super().__init__(*args, directory=directory or str(DEMO_DIR), **kwargs)

  def end_headers(self) -> None:
    self.send_header('Cache-Control', 'no-store')
    super().end_headers()

  def log_message(self, format: str, *args) -> None:
    print(f"[demo] {self.address_string()} - {format % args}", flush=True)


def bind_server(port: int, handler) -> tuple[ReusableTCPServer, int]:
  last_error: OSError | None = None
  for candidate in range(port, port + 10):
    try:
      httpd = ReusableTCPServer(('', candidate), handler)
      return httpd, candidate
    except OSError as exc:
      last_error = exc
  if last_error:
    raise last_error
  raise RuntimeError('Could not bind demo server')


def main() -> None:
  parser = argparse.ArgumentParser(description='WalkingStick localhost demo server')
  parser.add_argument('--port', type=int, default=DEFAULT_PORT, help='Port to bind')
  parser.add_argument('--no-open', action='store_true', help='Do not open a browser tab')
  args = parser.parse_args()

  if not DEMO_DIR.joinpath('index.html').exists():
    print('ERROR: demo files not found. Run from the WalkingStick repo root.', file=sys.stderr)
    print(f'Expected: {DEMO_DIR / "index.html"}', file=sys.stderr)
    raise SystemExit(1)

  os.chdir(DEMO_DIR)
  handler = functools.partial(DemoHandler, directory=str(DEMO_DIR))

  try:
    httpd, port = bind_server(args.port, handler)
  except OSError as exc:
    print(f'ERROR: Could not start server on port {args.port}: {exc}', file=sys.stderr)
    print('Try: python demo/server.py --port 8081', file=sys.stderr)
    raise SystemExit(1) from exc

  url = f'http://localhost:{port}'
  print('=' * 60, flush=True)
  print('WalkingStick demo is running', flush=True)
  print(f'Open in your browser: {url}', flush=True)
  print('Press Ctrl+C to stop.', flush=True)
  print('=' * 60, flush=True)

  if not args.no_open:
    try:
      webbrowser.open(url)
    except Exception:
      print('Could not open a browser automatically. Paste the URL above manually.', flush=True)

  try:
    httpd.serve_forever()
  except KeyboardInterrupt:
    print('\nDemo server stopped.', flush=True)
  finally:
    httpd.server_close()


if __name__ == '__main__':
  main()
