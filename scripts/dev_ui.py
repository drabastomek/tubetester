#!/usr/bin/env python3
"""
Restart the VTTester UI when source files change (stdlib-only file watcher).

Usage (from repo root):

  PYTHONPATH=src python scripts/dev_ui.py

Optional:

  PYTHONPATH=src python scripts/dev_ui.py --watch src/backend/database
"""

from __future__ import annotations

import argparse
import os
import signal
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_WATCH = [ROOT / "src" / "frontend"]


def _latest_mtime(paths: list[Path]) -> float:
    latest = 0.0
    for base in paths:
        if not base.exists():
            continue
        for file in base.rglob("*.py"):
            try:
                latest = max(latest, file.stat().st_mtime)
            except OSError:
                continue
    return latest


def _start_app() -> subprocess.Popen[bytes]:
    env = os.environ.copy()
    env["PYTHONPATH"] = str(ROOT / "src")
    print("\n--- starting: python -m frontend ---", flush=True)
    return subprocess.Popen(
        [sys.executable, "-m", "frontend"],
        cwd=ROOT,
        env=env,
    )


def _stop_app(proc: subprocess.Popen[bytes] | None) -> None:
    if proc is None or proc.poll() is not None:
        return
    print("--- stopping app ---", flush=True)
    proc.send_signal(signal.SIGTERM)
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Run VTTester UI with auto-restart on save")
    parser.add_argument(
        "--watch",
        action="append",
        type=Path,
        help="Extra directory to watch (may be repeated)",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=0.4,
        help="Poll interval in seconds (default: 0.4)",
    )
    args = parser.parse_args(argv)

    watch_paths = list(DEFAULT_WATCH)
    if args.watch:
        watch_paths.extend(p.resolve() for p in args.watch)

    print("Watching:")
    for path in watch_paths:
        print(f"  {path.relative_to(ROOT)}")
    print("Save a .py file to reload. Ctrl+C to quit.", flush=True)

    last_mtime = 0.0
    proc: subprocess.Popen[bytes] | None = None

    try:
        while True:
            current = _latest_mtime(watch_paths)
            if proc is None or current > last_mtime:
                last_mtime = current
                _stop_app(proc)
                proc = _start_app()
                if proc.poll() is not None:
                    print(f"App exited with code {proc.returncode}", flush=True)
            time.sleep(args.interval)
    except KeyboardInterrupt:
        print("\nShutting down.", flush=True)
        _stop_app(proc)
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
