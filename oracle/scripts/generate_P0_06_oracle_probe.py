#!/usr/bin/env python3
"""Generate the P0.06 reusable pinned-PyQtGraph oracle probe fixture."""

from __future__ import annotations

import argparse
import ast
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
FIXTURE_PATH = Path("oracle/fixtures/P0_06/probe_contract.json")
MISMATCH_EXAMPLE_PATH = Path("oracle/fixtures/P0_06/mismatch_failure_example.txt")
REQUIRED_LOCK_KEYS = ("repo", "ref", "pinned_commit", "docs_url", "checkout_path")
SCHEMA_VERSION = 1
ISSUE = "P0.06"

INPUTS = {
    "description": "Reusable pinned-PyQtGraph oracle probe template sanity values",
    "values": [1.25, -2.5, 3.75],
    "scale": 2.0,
    "offset": 0.5,
}
TOLERANCE = {
    "absolute": 0.0,
    "relative": 0.0,
    "policy": "exact JSON numeric comparison for deterministic template outputs",
}

REFERENCE_PROBE = r"""
import ast
import json
import math
import sys
import types
from pathlib import Path


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

    @staticmethod
    def dotProduct(left, right):
        return (left.x() * right.x()) + (left.y() * right.y())


QtCore = types.SimpleNamespace(
    QPointF=QPointF,
    QPoint=QPointF,
    QSize=QSize,
    QSizeF=QSize,
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


payload = json.loads(sys.stdin.read())
checkout = Path(payload["checkout_path"]).resolve()
values = [float(value) for value in payload["inputs"]["values"]]
scale = float(payload["inputs"]["scale"])
offset = float(payload["inputs"]["offset"])

# Load only the Point class from the pinned source with minimal Qt/math shims.
# This avoids importing pyqtgraph.__init__, NumPy, or real Qt bindings in clean
# project environments while still deriving the template values from upstream
# PyQtGraph behavior.
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

points = [Point(value, offset) for value in values]
scaled_values = [(point.x() * scale) + point.y() for point in points]
print(
    json.dumps(
        {"scaled_values": scaled_values, "sum": sum(scaled_values), "count": len(values)},
        sort_keys=True,
    )
)
"""


def load_lock(root: Path) -> dict[str, str]:
    lock_path = root / LOCK_PATH
    if not lock_path.exists():
        raise SystemExit(f"missing reference lock: {LOCK_PATH.as_posix()}")
    data = yaml.safe_load(lock_path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise SystemExit(f"invalid reference lock: {LOCK_PATH.as_posix()}")
    missing = [key for key in REQUIRED_LOCK_KEYS if not data.get(key)]
    if missing:
        raise SystemExit(f"reference lock missing required keys: {', '.join(missing)}")
    return {key: str(data[key]) for key in REQUIRED_LOCK_KEYS}


def run_git(checkout: Path, *args: str) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=checkout,
        text=True,
        capture_output=True,
        check=False,
    )
    if result.returncode != 0:
        raise SystemExit(result.stderr.strip() or f"git {' '.join(args)} failed")
    return result.stdout.strip()


def verify_checkout(checkout: Path, lock: dict[str, str]) -> None:
    actual_commit = run_git(checkout, "rev-parse", "HEAD")
    if actual_commit != lock["pinned_commit"]:
        raise SystemExit(
            "pinned PyQtGraph checkout mismatch: "
            f"expected {lock['pinned_commit']} but found {actual_commit}"
        )
    dirty = run_git(checkout, "status", "--porcelain")
    if dirty:
        raise SystemExit(f"pinned PyQtGraph checkout is dirty: {lock['checkout_path']}")


def clone_pinned_reference(lock: dict[str, str], destination: Path) -> None:
    try:
        run_git(
            destination.parent,
            "clone",
            "--quiet",
            "--depth",
            "1",
            "--filter=blob:none",
            "--no-checkout",
            "--branch",
            lock["ref"],
            "--single-branch",
            lock["repo"],
            str(destination),
        )
        run_git(destination, "checkout", "--quiet", "--detach", lock["pinned_commit"])
    except SystemExit as exc:
        raise SystemExit(
            "reference checkout is absent and pinned-source fallback failed: "
            f"could not materialize {lock['ref']} at {lock['pinned_commit']} "
            f"from {lock['repo']}: {exc}"
        ) from exc
    verify_checkout(destination, lock)


@contextmanager
def resolve_checkout(root: Path, lock: dict[str, str]) -> Iterator[Path]:
    checkout = root / lock["checkout_path"]
    if checkout.is_dir():
        verify_checkout(checkout, lock)
        yield checkout
        return
    with tempfile.TemporaryDirectory(prefix="pyqtgraph-P0_06-oracle-") as temp_dir:
        fallback_checkout = Path(temp_dir) / "pyqtgraph"
        clone_pinned_reference(lock, fallback_checkout)
        yield fallback_checkout


def read_pyqtgraph_version(checkout: Path) -> str:
    init_path = checkout / "pyqtgraph" / "__init__.py"
    if not init_path.exists():
        raise SystemExit(f"missing pyqtgraph package in pinned checkout: {init_path}")
    tree = ast.parse(init_path.read_text(encoding="utf-8"), filename=str(init_path))
    for node in tree.body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name) and target.id == "__version__":
                    value = ast.literal_eval(node.value)
                    return str(value)
    raise SystemExit(f"could not read pyqtgraph.__version__ from {init_path}")


def run_reference_probe(checkout: Path) -> dict[str, Any]:
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    env["PYTHONDONTWRITEBYTECODE"] = "1"
    payload = {"checkout_path": str(checkout), "inputs": INPUTS}
    result = subprocess.run(
        [sys.executable, "-c", REFERENCE_PROBE],
        text=True,
        input=json.dumps(payload),
        capture_output=True,
        env=env,
        timeout=30,
        check=False,
    )
    if result.returncode != 0:
        raise SystemExit(result.stderr.strip() or "P0.06 reference probe failed")
    return json.loads(result.stdout)


def build_fixture(root: Path) -> dict[str, Any]:
    lock = load_lock(root)
    with resolve_checkout(root, lock) as checkout:
        version = read_pyqtgraph_version(checkout)
        expected = run_reference_probe(checkout)
    return {
        "schema_version": SCHEMA_VERSION,
        "issue": ISSUE,
        "reference": {
            **lock,
            "pyqtgraph_version": version,
            "pyqtgraph_commit": lock["pinned_commit"],
        },
        "inputs": INPUTS,
        "expected": expected,
        "tolerance": TOLERANCE,
    }


def json_text(data: dict[str, Any]) -> str:
    return json.dumps(data, indent=2, sort_keys=True) + "\n"


def scalar_equal(left: Any, right: Any) -> bool:
    return left == right


def find_mismatch(
    expected: Any, actual: Any, path: str = "$"
) -> tuple[str, Any, Any] | None:
    if isinstance(expected, dict) and isinstance(actual, dict):
        for key in sorted(set(expected) | set(actual)):
            if key not in expected:
                return f"{path}.{key}", "<missing>", actual[key]
            if key not in actual:
                return f"{path}.{key}", expected[key], "<missing>"
            mismatch = find_mismatch(expected[key], actual[key], f"{path}.{key}")
            if mismatch is not None:
                return mismatch
        return None
    if isinstance(expected, list) and isinstance(actual, list):
        for index in range(max(len(expected), len(actual))):
            if index >= len(expected):
                return f"{path}[{index}]", "<missing>", actual[index]
            if index >= len(actual):
                return f"{path}[{index}]", expected[index], "<missing>"
            mismatch = find_mismatch(expected[index], actual[index], f"{path}[{index}]")
            if mismatch is not None:
                return mismatch
        return None
    if not scalar_equal(expected, actual):
        return path, expected, actual
    return None


def mismatch_message(path: Path, json_path: str, expected: Any, actual: Any) -> str:
    return (
        "oracle fixture mismatch\n"
        f"fixture: {path.as_posix()}\n"
        f"path: {json_path}\n"
        f"expected fixture value: {expected!r}\n"
        f"actual probe value: {actual!r}\n"
        "tolerance absolute=0.0 relative=0.0\n"
        "Regenerate with: python3 oracle/scripts/generate_P0_06_oracle_probe.py --emit-mismatch-example\n"
    )


def write_mismatch_example(root: Path, fixture: dict[str, Any]) -> None:
    actual = json.loads(json_text(fixture))
    stale = json.loads(json_text(fixture))
    stale["expected"]["scaled_values"][0] = -999.0
    mismatch = find_mismatch(stale, actual)
    if mismatch is None:
        raise SystemExit("could not construct P0.06 mismatch example")
    json_path, expected, actual_value = mismatch
    path = root / MISMATCH_EXAMPLE_PATH
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        mismatch_message(FIXTURE_PATH, json_path, expected, actual_value),
        encoding="utf-8",
    )


def write_fixture(
    root: Path, fixture: dict[str, Any], emit_mismatch_example: bool
) -> None:
    path = root / FIXTURE_PATH
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json_text(fixture), encoding="utf-8")
    if emit_mismatch_example:
        write_mismatch_example(root, fixture)


def check_fixture(root: Path, fixture: dict[str, Any]) -> None:
    path = root / FIXTURE_PATH
    if not path.exists():
        raise SystemExit(f"missing P0.06 fixture: {FIXTURE_PATH.as_posix()}")
    current = json.loads(path.read_text(encoding="utf-8"))
    mismatch = find_mismatch(current, fixture)
    if mismatch is not None:
        json_path, expected, actual = mismatch
        sys.stderr.write(mismatch_message(FIXTURE_PATH, json_path, expected, actual))
        raise SystemExit(1)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path.cwd(), help="repository root")
    parser.add_argument(
        "--check", action="store_true", help="fail if committed fixture is stale"
    )
    parser.add_argument(
        "--format",
        choices=("yaml", "json"),
        default="yaml",
        help="stdout manifest format",
    )
    parser.add_argument(
        "--emit-mismatch-example",
        action="store_true",
        help="write the representative stale-fixture mismatch diagnostic artifact",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = args.root.resolve()
    fixture = build_fixture(root)
    status = "current" if args.check else "written"
    if args.check:
        check_fixture(root, fixture)
        message = "P0.06 oracle fixture is current"
    else:
        write_fixture(root, fixture, args.emit_mismatch_example)
        message = "P0.06 oracle fixture written"

    manifest = {
        "fixture": FIXTURE_PATH.as_posix(),
        "mismatch_example": MISMATCH_EXAMPLE_PATH.as_posix(),
        "issue": ISSUE,
        "status": status,
    }
    if args.format == "json":
        sys.stdout.write(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    else:
        sys.stdout.write(yaml.safe_dump(manifest, sort_keys=False))
    if args.check:
        sys.stdout.write(message + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
