#!/usr/bin/env python3
"""Serve the WalkingStick localhost demo."""

from __future__ import annotations

import argparse
import functools
import http.server
import os
import socketserver
import webbrowser
from pathlib import Path

DEMO_DIR = Path(__file__).resolve().parent
DEFAULT_PORT = 8080


class DemoHandler(http.server.SimpleHTTPRequestHandler):
  def __init__(self, *args, directory: str | None = None, **kwargs):
    super().__init__(*args, directory=directory or str(DEMO_DIR), **kwargs)

  def end_headers(self) -> None:
    self.send_header('Cache-Control', 'no-store')
    super().end_headers()

  def log_message(self, format: str, *args) -> None:
    print(f"[demo] {self.address_string()} - {format % args}")


def main() -> None:
  parser = argparse.ArgumentParser(description='WalkingStick localhost demo server')
  parser.add_argument('--port', type=int, default=DEFAULT_PORT, help='Port to bind')
  parser.add_argument('--no-open', action='store_true', help='Do not open a browser tab')
  args = parser.parse_args()

  os.chdir(DEMO_DIR)
  handler = functools.partial(DemoHandler, directory=str(DEMO_DIR))

  with socketserver.TCPServer(('', args.port), handler) as httpd:
    url = f'http://localhost:{args.port}'
    print(f'WalkingStick demo running at {url}')
    print('Press Ctrl+C to stop.')

    if not args.no_open:
      webbrowser.open(url)

    try:
      httpd.serve_forever()
    except KeyboardInterrupt:
      print('\nDemo server stopped.')


if __name__ == '__main__':
  main()
