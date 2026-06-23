"""
Scenario runner for VTTester protocol harness (desktop ↔ MCU).

  PYTHONPATH=src python -m backend.communication.harness_client /dev/cu.usbserial-0001
  PYTHONPATH=src python -m backend.communication.harness_client /dev/cu.usbserial-0001 --scenario out_of_range
"""

from __future__ import annotations

import argparse
import sys
import time
from collections.abc import Callable
from dataclasses import dataclass
from typing import Any

import serial

from backend.communication.mcu_serial import VTTesterSerial
from backend.communication.protocol import Data, OutOfRangeError


@dataclass
class Step:
    name: str
    tx: bytes | Callable[[], bytes]
    expect: type | str | None = None
    expect_fields: dict[str, Any] | None = None
    timeout_s: float = 0.5
    delay_before_tx_s: float = 0.0


class ScenarioError(AssertionError):
    pass


def format_hex(data: bytes) -> str:
    return " ".join(f"{b:02x}" for b in data)


def _check_fields(data: Data, expect_fields: dict[str, Any]) -> None:
    for key, expected in expect_fields.items():
        actual = getattr(data, key)
        if actual != expected:
            raise ScenarioError(
                f"field {key}: expected {expected!r}, got {actual!r}"
            )


def run_step(link: VTTesterSerial, step: Step) -> Any:
    if step.delay_before_tx_s > 0:
        time.sleep(step.delay_before_tx_s)

    tx = step.tx() if callable(step.tx) else step.tx
    link.send_message(tx)
    response = link.read_response(timeout_s=step.timeout_s)

    if step.expect is None:
        if response is not None:
            raise ScenarioError(
                f"{step.name}: expected no response, got {response!r}"
            )
        return None

    if isinstance(step.expect, str):
        if response != step.expect:
            raise ScenarioError(
                f"{step.name}: expected {step.expect!r}, got {response!r}"
            )
        return response

    if not isinstance(response, step.expect):
        raise ScenarioError(
            f"{step.name}: expected {step.expect.__name__}, got {type(response).__name__}: {response!r}"
        )

    if step.expect_fields:
        if isinstance(response, Data):
            _check_fields(response, step.expect_fields)
        elif isinstance(response, OutOfRangeError):
            for key, expected in step.expect_fields.items():
                actual = getattr(response, key)
                if actual != expected:
                    raise ScenarioError(
                        f"{step.name}: {key}: expected {expected!r}, got {actual!r}"
                    )

    return response


def run_scenario(link: VTTesterSerial, steps: list[Step]) -> list[Any]:
    results: list[Any] = []
    for step in steps:
        results.append(run_step(link, step))
    return results


def main(argv: list[str] | None = None) -> int:
    from backend.communication.scenarios import SCENARIOS

    p = argparse.ArgumentParser(description="Run VTTester harness scenarios")
    p.add_argument("port", help="Serial device (e.g. /dev/cu.usbserial-0001)")
    p.add_argument(
        "--scenario",
        choices=sorted(SCENARIOS),
        default="nominal",
        help="Scenario to run (default: nominal)",
    )
    p.add_argument("-b", "--baud", type=int, default=9600)
    args = p.parse_args(argv)

    steps = SCENARIOS[args.scenario]

    try:
        with VTTesterSerial(args.port, baudrate=args.baud) as link:
            link.drain()
            print(f"Running scenario: {args.scenario} ({len(steps)} steps)")
            for i, step in enumerate(steps, 1):
                print(f"  [{i}/{len(steps)}] {step.name}...", end=" ", flush=True)
                result = run_step(link, step)
                print("ok" if result is not None else "ok (no response)")
                if isinstance(result, Data):
                    print(result)
                elif result is not None:
                    print(f"    -> {result!r}")
            print("PASS")
    except ScenarioError as e:
        print(f"FAIL: {e}", file=sys.stderr)
        return 1
    except serial.SerialException as e:
        print(e, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
