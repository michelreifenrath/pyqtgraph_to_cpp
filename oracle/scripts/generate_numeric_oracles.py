#!/usr/bin/env python3
"""Generate deterministic numeric oracle fixtures from the pinned reference."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile
from collections.abc import Iterator
from contextlib import contextmanager
from pathlib import Path
from typing import Any

import yaml

LOCK_PATH = Path("reference/source.lock")
FIXTURE_PATH = Path("oracle/fixtures/numeric")
REQUIRED_LOCK_KEYS = ("repo", "ref", "pinned_commit", "docs_url", "checkout_path")
SCHEMA_VERSION = 1

AFFINE_INPUTS = {
    "points": [[0.0, 0.0], [1.0, 2.0], [-3.0, 4.5]],
    "scale": [2.0, 3.0],
    "offset": [1.5, -2.0],
}
LOG_INPUTS = {"values": [0.1, 1.0, 10.0, 100.0], "base": 10.0}

REFERENCE_PROBE = r"""
import ast
import json
import math
import sys
import types
import warnings
from pathlib import Path

import numpy as np


class QSize:
    def __init__(self, width=0.0, height=0.0):
        self._width = float(width)
        self._height = float(height)

    def width(self):
        return self._width

    def height(self):
        return self._height


class QPointF:
    def __init__(self, *args):
        if not args:
            self._x = 0.0
            self._y = 0.0
        elif len(args) == 1 and isinstance(args[0], QPointF):
            self._x = args[0].x()
            self._y = args[0].y()
        elif len(args) == 1 and hasattr(args[0], "__getitem__"):
            self._x = float(args[0][0])
            self._y = float(args[0][1])
        elif len(args) == 2:
            self._x = float(args[0])
            self._y = float(args[1])
        else:
            raise TypeError("QPointF requires zero, one point-like, or two coordinates")

    def x(self):
        return self._x

    def y(self):
        return self._y

    def setX(self, value):
        self._x = float(value)

    def setY(self, value):
        self._y = float(value)


class QRectF:
    def __init__(self, *args):
        self.args = args


QtCore = types.SimpleNamespace(
    QPointF=QPointF,
    QSize=QSize,
    QSizeF=QSize,
    QRectF=QRectF,
)


def require_under_checkout(source_path, checkout_path):
    resolved = Path(source_path).resolve()
    try:
        resolved.relative_to(checkout_path)
    except ValueError as exc:
        raise RuntimeError(
            f"reference source {resolved} is outside pinned checkout {checkout_path}"
        ) from exc


def reference_class(relative_path, class_name, namespace):
    source_path = (checkout / relative_path).resolve()
    require_under_checkout(source_path, checkout)
    if not source_path.is_file():
        raise RuntimeError(f"missing PyQtGraph reference source: {relative_path}")

    tree = ast.parse(source_path.read_text(encoding="utf-8"), filename=str(source_path))
    class_node = next(
        (
            node
            for node in tree.body
            if isinstance(node, ast.ClassDef) and node.name == class_name
        ),
        None,
    )
    if class_node is None:
        raise RuntimeError(
            f"missing PyQtGraph reference class {class_name}: {relative_path}"
        )

    module = ast.Module(body=[class_node], type_ignores=[])
    ast.fix_missing_locations(module)
    exec(compile(module, str(source_path), "exec"), namespace)
    return namespace[class_name]


def point_pair(point):
    return [float(point[0]), float(point[1])]


payload = json.loads(sys.stdin.read())
checkout = Path(payload["checkout_path"]).resolve()

affine_inputs = payload["affine_inputs"]
log_inputs = payload["log_inputs"]
if float(log_inputs["base"]) != 10.0:
    raise RuntimeError("PyQtGraph PlotDataItem log mode only supports base-10 mapping")

Point = reference_class(
    "pyqtgraph/Point.py",
    "Point",
    {
        "QtCore": QtCore,
        "atan2": math.atan2,
        "degrees": math.degrees,
        "hypot": math.hypot,
    },
)
PlotDataset = reference_class(
    "pyqtgraph/graphicsItems/PlotDataItem.py",
    "PlotDataset",
    {
        "QtCore": QtCore,
        "RuntimeWarning": RuntimeWarning,
        "math": math,
        "np": np,
        "warnings": warnings,
    },
)

affine_points = []
for point in affine_inputs["points"]:
    mapped = Point(point) * Point(affine_inputs["scale"]) + Point(affine_inputs["offset"])
    affine_points.append(point_pair(mapped))

values = np.asarray(log_inputs["values"], dtype=float)
dataset = PlotDataset(values.copy(), values.copy())
dataset.applyLogMapping((True, False))
log_values = [float(value) for value in dataset.x.tolist()]

print(
    json.dumps(
        {
            "affine_transform": {"points": affine_points},
            "log_mapping": {"values": log_values},
        },
        allow_nan=False,
    )
)
"""


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
        help="validate numeric fixture generation without writing persistent files",
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


def run_git(args: list[str], *, cwd: Path | None = None) -> str:
    try:
        result = subprocess.run(
            ["git", *args],
            cwd=cwd,
            text=True,
            capture_output=True,
            check=False,
        )
    except OSError as exc:
        raise NumericOracleError(f"unable to run git {' '.join(args)}: {exc}") from exc

    if result.returncode != 0:
        detail = result.stderr.strip() or result.stdout.strip() or "git command failed"
        raise NumericOracleError(f"git {' '.join(args)} failed: {detail}")
    return result.stdout.strip()


def clone_pinned_reference(lock: dict[str, str], destination: Path, root: Path) -> None:
    ref = lock["ref"]
    pinned_commit = lock["pinned_commit"]
    try:
        run_git(
            [
                "clone",
                "--quiet",
                "--depth",
                "1",
                "--filter=blob:none",
                "--no-checkout",
                "--branch",
                ref,
                "--single-branch",
                lock["repo"],
                str(destination),
            ]
        )
        run_git(["checkout", "--quiet", "--detach", pinned_commit], cwd=destination)
    except NumericOracleError as exc:
        raise NumericOracleError(
            "reference checkout is absent and pinned-source fallback failed: "
            f"could not materialize {ref} at {pinned_commit} from {lock['repo']}: {exc}"
        ) from exc
    verify_reference_commit(destination, lock, root)


@contextmanager
def resolve_reference_checkout(root: Path, lock: dict[str, str]) -> Iterator[Path]:
    checkout = Path(lock["checkout_path"])
    checkout = checkout if checkout.is_absolute() else root / checkout
    checkout = checkout.resolve()
    if checkout.is_dir():
        verify_reference_commit(checkout, lock, root)
        yield checkout
        return

    with tempfile.TemporaryDirectory(prefix="pyqtgraph-numeric-oracle-") as temp_dir:
        fallback_checkout = Path(temp_dir) / "pyqtgraph"
        clone_pinned_reference(lock, fallback_checkout, root)
        yield fallback_checkout


def verify_reference_commit(checkout: Path, lock: dict[str, str], root: Path) -> None:
    try:
        result = subprocess.run(
            ["git", "-C", str(checkout), "rev-parse", "HEAD"],
            text=True,
            capture_output=True,
            check=False,
        )
    except OSError as exc:
        raise NumericOracleError(
            f"unable to inspect reference checkout with git: {exc}"
        ) from exc

    if result.returncode != 0:
        detail = (result.stderr or result.stdout).strip()
        message = (
            "reference checkout is not a usable git checkout: "
            f"{posix_relative(checkout, root)}"
        )
        if detail:
            message += f" ({detail})"
        raise NumericOracleError(message)

    actual_commit = result.stdout.strip()
    expected_commit = lock["pinned_commit"]
    if actual_commit != expected_commit:
        raise NumericOracleError(
            "reference checkout commit mismatch: "
            f"{posix_relative(checkout, root)} "
            f"(expected {expected_commit}, got {actual_commit})"
        )


def run_reference_probe(root: Path, checkout: Path) -> dict[str, Any]:
    env = os.environ.copy()
    existing_pythonpath = env.get("PYTHONPATH")
    env["PYTHONPATH"] = str(checkout) + (
        os.pathsep + existing_pythonpath if existing_pythonpath else ""
    )
    env.setdefault("QT_QPA_PLATFORM", "offscreen")
    env["PYTHONDONTWRITEBYTECODE"] = "1"

    payload = {
        "checkout_path": str(checkout),
        "affine_inputs": AFFINE_INPUTS,
        "log_inputs": LOG_INPUTS,
    }
    try:
        result = subprocess.run(
            [sys.executable, "-c", REFERENCE_PROBE],
            cwd=root,
            env=env,
            input=json.dumps(payload),
            text=True,
            capture_output=True,
            check=False,
            timeout=30,
        )
    except subprocess.TimeoutExpired as exc:
        raise NumericOracleError(
            "unable to use pinned PyQtGraph reference: reference probe timed out"
        ) from exc
    except OSError as exc:
        raise NumericOracleError(
            f"unable to use pinned PyQtGraph reference: {exc}"
        ) from exc

    if result.returncode != 0:
        detail = (result.stderr or result.stdout).strip()
        message = "unable to use pinned PyQtGraph reference runtime"
        if detail:
            message += f": {detail}"
        raise NumericOracleError(message)

    try:
        parsed = json.loads(result.stdout)
    except json.JSONDecodeError as exc:
        raise NumericOracleError(
            "unable to use pinned PyQtGraph reference runtime: "
            "reference probe returned invalid JSON"
        ) from exc

    if not isinstance(parsed, dict):
        raise NumericOracleError(
            "unable to use pinned PyQtGraph reference runtime: "
            "reference probe returned invalid payload"
        )
    return parsed


def case_definitions(
    lock: dict[str, str], reference_results: dict[str, Any]
) -> list[dict[str, Any]]:
    reference = {"ref": lock["ref"], "pinned_commit": lock["pinned_commit"]}

    try:
        affine_expected = reference_results["affine_transform"]
        log_expected = reference_results["log_mapping"]
    except KeyError as exc:
        raise NumericOracleError(
            "unable to use pinned PyQtGraph reference runtime: "
            f"missing result for {exc.args[0]}"
        ) from exc

    return [
        {
            "schema_version": SCHEMA_VERSION,
            "case": "affine_transform",
            "reference": reference,
            "inputs": AFFINE_INPUTS,
            "expected": affine_expected,
            "tolerance": {"absolute": 0.0, "relative": 0.0},
        },
        {
            "schema_version": SCHEMA_VERSION,
            "case": "log_mapping",
            "reference": reference,
            "inputs": LOG_INPUTS,
            "expected": log_expected,
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
    expected_by_path = {
        fixture_path(fixtures_dir, case): fixture_text(case) for case in cases
    }
    if not fixtures_dir.exists():
        return
    if not fixtures_dir.is_dir():
        raise NumericOracleError(
            f"numeric fixture path is not a directory: {posix_relative(fixtures_dir, root)}"
        )

    existing_json_paths = set(fixtures_dir.rglob("*.json"))
    if not existing_json_paths:
        return

    for path, expected in sorted(
        expected_by_path.items(), key=lambda item: item[0].as_posix()
    ):
        if not path.is_file():
            raise NumericOracleError(
                f"missing numeric fixture: {posix_relative(path, root)}"
            )
        if path.read_text(encoding="utf-8") != expected:
            raise NumericOracleError(
                f"stale numeric fixture: {posix_relative(path, root)}"
            )

    for path in sorted(existing_json_paths):
        if path not in expected_by_path:
            raise NumericOracleError(
                f"unknown numeric fixture: {posix_relative(path, root)}"
            )


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
        with resolve_reference_checkout(root, lock) as checkout:
            reference_results = run_reference_probe(root, checkout)
        cases = case_definitions(lock, reference_results)
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
