#!/usr/bin/env python3
"""Generate deterministic numeric oracle fixtures for the pinned reference."""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path
from typing import Any

import yaml

LOCK_PATH = Path("reference/source.lock")
FIXTURE_PATH = Path("oracle/fixtures/numeric")
REQUIRED_LOCK_KEYS = ("repo", "ref", "pinned_commit", "docs_url", "checkout_path")
SCHEMA_VERSION = 1


class NumericOracleError(RuntimeError):
    """Raised when numeric oracle fixture generation cannot be completed."""


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate deterministic numeric oracle fixtures."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=Path.cwd(),
        help="repository root containing reference/source.lock",
    )
    parser.add_argument(
        "--fixtures-dir",
        type=Path,
        default=FIXTURE_PATH,
        help="numeric oracle fixture directory, relative to --root unless absolute",
    )
    parser.add_argument(
        "--format",
        choices=("yaml", "json"),
        default="yaml",
        help="manifest serialization format for stdout",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="validate existing numeric fixtures without writing persistent files",
    )
    return parser.parse_args(argv)


def load_lock(root: Path) -> dict[str, str]:
    lock_file = root / LOCK_PATH
    if not lock_file.is_file():
        raise NumericOracleError(f"missing reference lock: {LOCK_PATH.as_posix()}")

    data = yaml.safe_load(lock_file.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise NumericOracleError(f"invalid reference lock: {LOCK_PATH.as_posix()}")

    missing = [key for key in REQUIRED_LOCK_KEYS if not data.get(key)]
    if missing:
        raise NumericOracleError(
            "reference lock is missing required field(s): " + ", ".join(missing)
        )

    return {key: str(data[key]) for key in REQUIRED_LOCK_KEYS}


def resolve_fixtures_dir(root: Path, fixtures_dir: Path) -> Path:
    return fixtures_dir if fixtures_dir.is_absolute() else root / fixtures_dir


def posix_relative(path: Path, root: Path) -> str:
    try:
        return path.relative_to(root).as_posix()
    except ValueError:
        return path.as_posix()


def affine_transform_points(
    points: list[list[float]], scale: list[float], offset: list[float]
) -> list[list[float]]:
    return [
        [point[0] * scale[0] + offset[0], point[1] * scale[1] + offset[1]]
        for point in points
    ]


def log_mapping(values: list[float], base: float) -> list[float]:
    return [round(math.log(value, base), 12) for value in values]


def case_definitions(lock: dict[str, str]) -> list[dict[str, Any]]:
    reference = {"ref": lock["ref"], "pinned_commit": lock["pinned_commit"]}

    affine_inputs = {
        "points": [[0.0, 0.0], [1.0, 2.0], [-3.0, 4.5]],
        "scale": [2.0, 3.0],
        "offset": [1.5, -2.0],
    }
    log_inputs = {"values": [0.1, 1.0, 10.0, 100.0], "base": 10.0}

    return [
        {
            "schema_version": SCHEMA_VERSION,
            "case": "affine_transform",
            "reference": reference,
            "inputs": affine_inputs,
            "expected": {
                "points": affine_transform_points(
                    affine_inputs["points"], affine_inputs["scale"], affine_inputs["offset"]
                )
            },
            "tolerance": {"absolute": 0.0, "relative": 0.0},
        },
        {
            "schema_version": SCHEMA_VERSION,
            "case": "log_mapping",
            "reference": reference,
            "inputs": log_inputs,
            "expected": {"values": log_mapping(log_inputs["values"], log_inputs["base"])},
            "tolerance": {"absolute": 1.0e-12, "relative": 1.0e-12},
        },
    ]


def fixture_text(case: dict[str, Any]) -> str:
    return json.dumps(case, indent=2, sort_keys=False) + "\n"


def fixture_path(fixtures_dir: Path, case: dict[str, Any]) -> Path:
    return fixtures_dir / f"{case['case']}.json"


def write_if_changed(path: Path, text: str) -> None:
    if path.is_file() and path.read_text(encoding="utf-8") == text:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def write_fixtures(fixtures_dir: Path, cases: list[dict[str, Any]]) -> None:
    for case in cases:
        write_if_changed(fixture_path(fixtures_dir, case), fixture_text(case))


def check_existing_fixtures(
    fixtures_dir: Path, cases: list[dict[str, Any]], root: Path
) -> None:
    if not fixtures_dir.exists():
        return
    if not fixtures_dir.is_dir():
        raise NumericOracleError(
            f"numeric fixture path is not a directory: {posix_relative(fixtures_dir, root)}"
        )

    expected_by_path = {fixture_path(fixtures_dir, case): fixture_text(case) for case in cases}
    for path in sorted(fixtures_dir.rglob("*.json")):
        expected = expected_by_path.get(path)
        if expected is None:
            raise NumericOracleError(
                f"unknown numeric fixture: {posix_relative(path, root)}"
            )
        if path.read_text(encoding="utf-8") != expected:
            raise NumericOracleError(f"stale numeric fixture: {posix_relative(path, root)}")


def build_manifest(
    lock: dict[str, str], fixtures_dir: Path, cases: list[dict[str, Any]], root: Path
) -> dict[str, Any]:
    sorted_cases = sorted(cases, key=lambda case: str(case["case"]))
    return {
        "reference": {key: lock[key] for key in REQUIRED_LOCK_KEYS},
        "cases": [
            {
                "id": str(case["case"]),
                "fixture_path": posix_relative(fixture_path(fixtures_dir, case), root),
            }
            for case in sorted_cases
        ],
        "summary": {"case_count": len(sorted_cases)},
    }


def render_manifest(manifest: dict[str, Any], output_format: str) -> str:
    if output_format == "json":
        return json.dumps(manifest, indent=2, sort_keys=False) + "\n"
    return yaml.safe_dump(manifest, sort_keys=False)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    root = args.root.resolve()
    fixtures_dir = resolve_fixtures_dir(root, args.fixtures_dir).resolve()

    try:
        lock = load_lock(root)
        cases = case_definitions(lock)
        manifest = build_manifest(lock, fixtures_dir, cases, root)
        # Exercise serialization before check/write so format regressions fail early.
        rendered_manifest = render_manifest(manifest, args.format)
        if args.check:
            check_existing_fixtures(fixtures_dir, cases, root)
            print(f"numeric oracles verified ({len(cases)} cases)")
            return 0
        write_fixtures(fixtures_dir, cases)
        sys.stdout.write(rendered_manifest)
        return 0
    except NumericOracleError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
