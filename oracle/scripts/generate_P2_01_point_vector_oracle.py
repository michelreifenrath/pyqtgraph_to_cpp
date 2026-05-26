#!/usr/bin/env python3
"""Generate the P2.01 pinned-PyQtGraph Point/Vector oracle fixture."""

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
FIXTURE_PATH = Path("oracle/fixtures/P2_01/point_vector_semantics.json")
REQUIRED_LOCK_KEYS = ("repo", "ref", "pinned_commit", "docs_url", "checkout_path")
SCHEMA_VERSION = 1
ISSUE = "P2.01"
TOLERANCE = {
    "point_absolute": 1.0e-12,
    "point_relative": 0.0,
    "vector_absolute": 1.0e-5,
    "vector_relative": 0.0,
    "policy": "finite Point values compare with absolute 1e-12; QVector3D-backed Vector values compare with absolute 1e-5; NaN/None are asserted semantically",
}

REFERENCE_PROBE = r"""
import ast
import json
import math
import sys
import types
from pathlib import Path


class QSizeF:
    def __init__(self, width=0.0, height=0.0):
        self._width = float(width)
        self._height = float(height)

    def width(self):
        return self._width

    def height(self):
        return self._height


class QSize(QSizeF):
    pass


class QPointF:
    def __init__(self, *args):
        if not args:
            self._x = 0.0
            self._y = 0.0
        elif len(args) == 1 and isinstance(args[0], QPointF):
            self._x = float(args[0].x())
            self._y = float(args[0].y())
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

    def toPoint(self):
        return QPoint(round(self._x), round(self._y))

    @staticmethod
    def dotProduct(left, right):
        return left.x() * right.x() + left.y() * right.y()


class QPoint(QPointF):
    pass


class QVector3D:
    def __init__(self, *args):
        if not args:
            self._x = 0.0
            self._y = 0.0
            self._z = 0.0
        elif len(args) == 1 and isinstance(args[0], QVector3D):
            self._x = float(args[0].x())
            self._y = float(args[0].y())
            self._z = float(args[0].z())
        elif len(args) == 3:
            self._x = float(args[0])
            self._y = float(args[1])
            self._z = float(args[2])
        else:
            raise TypeError("QVector3D requires zero, one vector, or three coordinates")

    def x(self):
        return self._x

    def y(self):
        return self._y

    def z(self):
        return self._z

    def setX(self, value):
        self._x = float(value)

    def setY(self, value):
        self._y = float(value)

    def setZ(self, value):
        self._z = float(value)

    def length(self):
        return math.sqrt(self._x * self._x + self._y * self._y + self._z * self._z)

    @staticmethod
    def dotProduct(left, right):
        return left.x() * right.x() + left.y() * right.y() + left.z() * right.z()


QtCore = types.SimpleNamespace(QPointF=QPointF, QPoint=QPoint, QSize=QSize, QSizeF=QSizeF)
QtGui = types.SimpleNamespace(QVector3D=QVector3D)
fn = types.SimpleNamespace(clip_scalar=lambda value, lower, upper: max(lower, min(upper, value)))


def require_under_checkout(source_path, checkout_path):
    resolved = Path(source_path).resolve()
    try:
        resolved.relative_to(checkout_path)
    except ValueError as exc:
        raise RuntimeError(f"reference source {resolved} is outside pinned checkout {checkout_path}") from exc


def reference_class(relative_path, class_name, namespace):
    source_path = (checkout / relative_path).resolve()
    require_under_checkout(source_path, checkout)
    if not source_path.is_file():
        raise RuntimeError(f"missing PyQtGraph reference source: {relative_path}")
    tree = ast.parse(source_path.read_text(encoding="utf-8"), filename=str(source_path))
    class_node = next((node for node in tree.body if isinstance(node, ast.ClassDef) and node.name == class_name), None)
    if class_node is None:
        raise RuntimeError(f"missing PyQtGraph reference class {class_name}: {relative_path}")
    module = ast.Module(body=[class_node], type_ignores=[])
    ast.fix_missing_locations(module)
    exec(compile(module, str(source_path), "exec"), namespace)
    return namespace[class_name]


def point2(point):
    return [point.x(), point.y()]


def vector3(vector):
    return [vector.x(), vector.y(), vector.z()]


def error_name(callable_):
    try:
        callable_()
    except Exception as exc:
        return type(exc).__name__
    return "<none>"


payload = json.loads(sys.stdin.read())
checkout = Path(payload["checkout_path"]).resolve()
Point = reference_class(
    "pyqtgraph/Point.py",
    "Point",
    {"QtCore": QtCore, "atan2": math.atan2, "degrees": math.degrees, "hypot": math.hypot},
)
Vector = reference_class(
    "pyqtgraph/Vector.py",
    "Vector",
    {"QtCore": QtCore, "QtGui": QtGui, "QT_LIB": "stub", "fn": fn, "acos": math.acos, "degrees": math.degrees},
)

p = Point(6.0, 8.0)
other = Point(2.0, 4.0)
indexed = Point(10.0, 20.0)
indexed[0] = -1.0
indexed[1] = -2.0
zero_norm_error = error_name(lambda: Point(0.0, 0.0).norm())
qpoint = Point(3.0, 4.0).toQPoint()

v = Vector(10.0, 20.0, 30.0)
v[0] = -1.0
v[1] = -2.0
v[2] = -3.0

expected = {
    "point_construct_scalar": point2(Point(3.0)),
    "point_construct_sequence": point2(Point([8.0, 9.0])),
    "point_invalid_empty_error": error_name(lambda: Point([])),
    "point_len": len(p),
    "point_initial_index_values": [Point(10.0, 20.0)[0], Point(10.0, 20.0)[1]],
    "point_set_values": point2(indexed),
    "point_iteration_values": list(Point(10.0, 20.0)),
    "point_add": point2(p + other),
    "point_sub": point2(p - other),
    "point_mul": point2(p * other),
    "point_div": point2(p / other),
    "point_reflected_add": point2(2.0 + p),
    "point_reflected_sub": point2(20.0 - p),
    "point_reflected_mul": point2(2.0 * p),
    "point_reflected_div": point2(24.0 / p),
    "point_pow_point": point2(Point(2.0, 3.0) ** Point(4.0, 2.0)),
    "point_pow_scalar": point2(Point(4.0, 9.0) ** 0.5),
    "point_reflected_pow": point2(2.0 ** Point(3.0, 4.0)),
    "point_pow_zero_negative_error": error_name(lambda: Point(0.0, 2.0) ** Point(-1.0, 2.0)),
    "point_pow_scalar_zero_negative_error": error_name(lambda: Point(0.0, 2.0) ** -1.0),
    "point_reflected_pow_zero_negative_error": error_name(lambda: 0.0 ** Point(-1.0, 2.0)),
    "point_pow_negative_fractional_error": error_name(lambda: Point(-1.0, 2.0) ** 0.5),
    "point_length": p.length(),
    "point_norm": point2(p.norm()),
    "point_zero_norm_error": zero_norm_error,
    "point_angle_degrees": Point(1.0, 0.0).angle(Point(0.0, 1.0)),
    "point_angle_radians": Point(1.0, 0.0).angle(Point(0.0, 1.0), "radians"),
    "point_dot": Point(3.0, 4.0).dot(Point(5.0, 6.0)),
    "point_cross": Point(3.0, 4.0).cross(Point(5.0, 6.0)),
    "point_proj": point2(Point(3.0, 4.0).proj(Point(10.0, 0.0))),
    "point_min": Point(3.0, -4.0).min(),
    "point_max": Point(3.0, -4.0).max(),
    "point_copy": point2(p.copy()),
    "point_to_qpoint": point2(qpoint),
    "vector_construct_2": vector3(Vector(1.5, -2.25)),
    "vector_construct_3": vector3(Vector(1.5, -2.25, 3.75)),
    "vector_construct_sequence_2": vector3(Vector([8.0, 9.0])),
    "vector_construct_sequence_3": vector3(Vector([8.0, 9.0, 10.0])),
    "vector_invalid_sequence_1_error": error_name(lambda: Vector([1.0])),
    "vector_len": len(v),
    "vector_initial_index_values": [Vector(10.0, 20.0, 30.0)[0], Vector(10.0, 20.0, 30.0)[1], Vector(10.0, 20.0, 30.0)[2]],
    "vector_set_values": vector3(v),
    "vector_iteration_values": list(Vector(10.0, 20.0, 30.0)),
    "vector_angle_right": Vector(1.0, 0.0, 0.0).angle(Vector(0.0, 1.0, 0.0)),
    "vector_angle_zero": Vector().angle(Vector(1.0, 0.0, 0.0)),
    "vector_abs": vector3(abs(Vector(-1.0, 2.0, -3.0))),
}
print(json.dumps(expected, sort_keys=True, allow_nan=False))
"""

EQUIVALENCES = [
    "Point/Vector Python len/index/iteration are represented in C++ by coordinateCount(), at(), set(), and operator[] where available; no range iterators are required for P2.01.",
    "Upstream Point __pow__/__rpow__ are represented by Point::pow(QPointF), Point::pow(double), and pyqtgraph::pow(double, const Point&); operator^ is intentionally unsupported because it is bitwise and precedence-misleading in C++.",
    "Python dynamic coercions and invalid positional arities that are impossible in statically typed C++ are covered by typed Qt constructors and initializer-list length validation.",
    "Python ZeroDivisionError from zero-length Point.norm() is represented by std::domain_error in C++.",
    "Python ZeroDivisionError/TypeError from Point power-domain failures is represented by std::domain_error in C++.",
]


def load_lock(root: Path) -> dict[str, str]:
    data = yaml.safe_load((root / LOCK_PATH).read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise SystemExit(f"invalid reference lock: {LOCK_PATH.as_posix()}")
    missing = [key for key in REQUIRED_LOCK_KEYS if not data.get(key)]
    if missing:
        raise SystemExit(f"reference lock missing required keys: {', '.join(missing)}")
    return {key: str(data[key]) for key in REQUIRED_LOCK_KEYS}


def run_git(checkout: Path, *args: str) -> str:
    result = subprocess.run(
        ["git", *args], cwd=checkout, text=True, capture_output=True, check=False
    )
    if result.returncode != 0:
        raise SystemExit(result.stderr.strip() or f"git {' '.join(args)} failed")
    return result.stdout.strip()


def verify_checkout(checkout: Path, lock: dict[str, str]) -> None:
    actual_commit = run_git(checkout, "rev-parse", "HEAD")
    if actual_commit != lock["pinned_commit"]:
        raise SystemExit(
            f"pinned PyQtGraph checkout mismatch: expected {lock['pinned_commit']} but found {actual_commit}"
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
            f"reference checkout is absent and pinned-source fallback failed for {lock['ref']} at {lock['pinned_commit']}: {exc}"
        ) from exc
    verify_checkout(destination, lock)


@contextmanager
def resolve_checkout(root: Path, lock: dict[str, str]) -> Iterator[Path]:
    checkout = root / lock["checkout_path"]
    if checkout.is_dir():
        verify_checkout(checkout, lock)
        yield checkout
        return
    with tempfile.TemporaryDirectory(prefix="pyqtgraph-P2_01-oracle-") as temp_dir:
        fallback_checkout = Path(temp_dir) / "pyqtgraph"
        clone_pinned_reference(lock, fallback_checkout)
        yield fallback_checkout


def read_pyqtgraph_version(checkout: Path) -> str:
    init_path = checkout / "pyqtgraph" / "__init__.py"
    tree = ast.parse(init_path.read_text(encoding="utf-8"), filename=str(init_path))
    for node in tree.body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name) and target.id == "__version__":
                    return str(ast.literal_eval(node.value))
    raise SystemExit(f"could not read pyqtgraph.__version__ from {init_path}")


def run_reference_probe(checkout: Path) -> dict[str, Any]:
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "offscreen"
    env["PYTHONDONTWRITEBYTECODE"] = "1"
    result = subprocess.run(
        [sys.executable, "-c", REFERENCE_PROBE],
        text=True,
        input=json.dumps({"checkout_path": str(checkout)}),
        capture_output=True,
        env=env,
        timeout=30,
        check=False,
    )
    if result.returncode != 0:
        raise SystemExit(result.stderr.strip() or "P2.01 reference probe failed")
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
            "source_paths": ["pyqtgraph/Point.py", "pyqtgraph/Vector.py"],
        },
        "tolerance": TOLERANCE,
        "cpp_equivalences": EQUIVALENCES,
        "expected": expected,
    }


def json_text(data: dict[str, Any]) -> str:
    return json.dumps(data, indent=2, sort_keys=True, allow_nan=False) + "\n"


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
    if expected != actual:
        return path, expected, actual
    return None


def check_fixture(root: Path, fixture: dict[str, Any]) -> None:
    path = root / FIXTURE_PATH
    if not path.exists():
        raise SystemExit(f"missing P2.01 fixture: {FIXTURE_PATH.as_posix()}")
    current = json.loads(path.read_text(encoding="utf-8"))
    mismatch = find_mismatch(current, fixture)
    if mismatch is not None:
        json_path, expected, actual = mismatch
        sys.stderr.write(
            "oracle fixture mismatch\n"
            f"fixture: {FIXTURE_PATH.as_posix()}\n"
            f"path: {json_path}\n"
            f"expected fixture value: {expected!r}\n"
            f"actual probe value: {actual!r}\n"
            "Regenerate with: python3 oracle/scripts/generate_P2_01_point_vector_oracle.py\n"
        )
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
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = args.root.resolve()
    fixture = build_fixture(root)
    if args.check:
        check_fixture(root, fixture)
        status = "current"
        message = "P2.01 oracle fixture is current"
    else:
        path = root / FIXTURE_PATH
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json_text(fixture), encoding="utf-8")
        status = "written"
        message = "P2.01 oracle fixture written"
    manifest = {"fixture": FIXTURE_PATH.as_posix(), "issue": ISSUE, "status": status}
    if args.format == "json":
        sys.stdout.write(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    else:
        sys.stdout.write(yaml.safe_dump(manifest, sort_keys=False))
    if args.check:
        sys.stdout.write(message + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
