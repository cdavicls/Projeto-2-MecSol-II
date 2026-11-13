#!/usr/bin/env python3
"""Run the example models and compare the results with analytical values."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path


def resolve(data: object, path: str) -> float:
    current = data
    for part in path.split("."):
        if part.endswith("]") and "[" in part:
            name, index = part[:-1].split("[")
            current = current[name][int(index)]
        else:
            current = current[part]
    return float(current)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cli", default="build/ms2_cli", help="Path to the ms2_cli executable")
    parser.add_argument("--samples", type=int, default=2001, help="Samples per element")
    parser.add_argument("--examples", default="examples", help="Examples directory")
    args = parser.parse_args()

    root = Path(__file__).resolve().parent.parent
    cli = (root / args.cli).resolve()
    examples_dir = (root / args.examples).resolve()

    if not cli.exists():
        print(f"error: CLI not found at {cli}; run scripts/build.sh first", file=sys.stderr)
        return 1

    example_dirs = sorted(path.parent for path in examples_dir.glob("*/input.json"))
    if not example_dirs:
        print(f"error: no examples found in {examples_dir}", file=sys.stderr)
        return 1

    failures = 0
    print(f"{'example':<32} {'check':<28} {'expected':>14} {'actual':>14} {'rel. error':>12}  status")
    print("-" * 116)

    for example_dir in example_dirs:
        input_path = example_dir / "input.json"
        expected_path = example_dir / "expected.json"
        output_path = example_dir / "output.json"

        if not expected_path.exists():
            print(f"{example_dir.name:<32} missing expected.json", file=sys.stderr)
            failures += 1
            continue

        subprocess.run(
            [str(cli), "--input", str(input_path), "--output", str(output_path),
             "--samples", str(args.samples)],
            check=True,
        )

        expected = json.loads(expected_path.read_text(encoding="utf-8"))
        actual = json.loads(output_path.read_text(encoding="utf-8"))
        tolerance = float(expected.get("tolerance", 1e-6))
        absolute_tolerance = float(expected.get("absolute_tolerance", 1e-9))

        for check, expected_value in expected["checks"].items():
            actual_value = resolve(actual, check)
            absolute_error = abs(actual_value - expected_value)
            relative_error = (
                absolute_error / abs(expected_value) if expected_value != 0.0 else absolute_error
            )
            passed = absolute_error <= absolute_tolerance or relative_error <= tolerance
            if not passed:
                failures += 1
            print(
                f"{example_dir.name:<32} {check:<28} {expected_value:>14.6e} "
                f"{actual_value:>14.6e} {relative_error:>12.3e}  "
                f"{'ok' if passed else 'FAIL'}"
            )

    print("-" * 116)
    if failures:
        print(f"{failures} check(s) failed", file=sys.stderr)
        return 1

    print("all checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
