#!/usr/bin/env python3
"""Generate/check the P2.01 Point/Vector oracle fixture.

The probe executes the pinned PyQtGraph 0.14.0 Point.py and Vector.py files
against minimal Qt stand-ins so the numeric Python behavior can be recorded
without requiring a Python Qt binding at test time.
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import math
import os
import sys
import types
from pathlib import Path
from typing import Any, Callable

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_REF_ROOT = ROOT / "reference" / "pyqtgraph"
FIXTURE = ROOT / "oracle" / "fixtures" / "P2_01" / "point_vector_oracle.json"
PINNED_REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"
UPSTREAM_FILES = [
    "pyqtgraph/Point.py",
    "pyqtgraph/Vector.py",
    "tests/test_Point.py",
    "tests/test_Vector.py",
]


class QPointF:
    def __init__(self, x: Any = 0.0, y: Any | None = None) -> None:
        if y is None:
            if isinstance(x, QPointF):
                self._x = float(x.x())
                self._y = float(x.y())
            elif isinstance(x, QPoint):
                self._x = float(x.x())
                self._y = float(x.y())
            else:
                self._x = float(x)
                self._y = float(x)
        else:
            self._x = float(x)
            self._y = float(y)

    def x(self) -> float:
        return self._x

    def y(self) -> float:
        return self._y

    def setX(self, value: Any) -> None:
        self._x = float(value)

    def setY(self, value: Any) -> None:
        self._y = float(value)

    def toPoint(self) -> "QPoint":
        return QPoint(round(self._x), round(self._y))

    @staticmethod
    def dotProduct(a: "QPointF", b: "QPointF") -> float:
        return a.x() * b.x() + a.y() * b.y()


class QPoint(QPointF):
    def __init__(self, x: Any = 0, y: Any | None = None) -> None:
        super().__init__(int(x), int(x if y is None else y))


class QSizeF:
    def __init__(self, width: Any = 0.0, height: Any = 0.0) -> None:
        self._width = float(width)
        self._height = float(height)

    def width(self) -> float:
        return self._width

    def height(self) -> float:
        return self._height


class QSize(QSizeF):
    def __init__(self, width: Any = 0, height: Any = 0) -> None:
        super().__init__(int(width), int(height))


class QVector3D:
    def __init__(self, x: Any = 0.0, y: Any | None = None, z: Any | None = None) -> None:
        if y is None and z is None:
            if isinstance(x, QVector3D):
                self._x = float(x.x())
                self._y = float(x.y())
                self._z = float(x.z())
            else:
                self._x = float(x)
                self._y = 0.0
                self._z = 0.0
        elif y is not None and z is not None:
            self._x = float(x)
            self._y = float(y)
            self._z = float(z)
        else:
            raise TypeError("QVector3D requires 0, 1, or 3 arguments")

    def x(self) -> float:
        return self._x

    def y(self) -> float:
        return self._y

    def z(self) -> float:
        return self._z

    def setX(self, value: Any) -> None:
        self._x = float(value)

    def setY(self, value: Any) -> None:
        self._y = float(value)

    def setZ(self, value: Any) -> None:
        self._z = float(value)

    def length(self) -> float:
        return math.sqrt(self._x * self._x + self._y * self._y + self._z * self._z)

    @staticmethod
    def dotProduct(a: "QVector3D", b: "QVector3D") -> float:
        return a.x() * b.x() + a.y() * b.y() + a.z() * b.z()

    def __eq__(self, other: object) -> bool:
        return isinstance(other, QVector3D) and vector_tuple(self) == vector_tuple(other)


def clip_scalar(value: float, minimum: float, maximum: float) -> float:
    return min(max(value, minimum), maximum)


def install_fake_pyqtgraph_package() -> None:
    package = types.ModuleType("pyqtgraph")
    package.__path__ = []
    qt_module = types.ModuleType("pyqtgraph.Qt")
    qtcore = types.SimpleNamespace(QPoint=QPoint, QPointF=QPointF, QSize=QSize, QSizeF=QSizeF)
    qtgui = types.SimpleNamespace(QVector3D=QVector3D)
    qt_module.QT_LIB = "probe-qt-standins"
    qt_module.QtCore = qtcore
    qt_module.QtGui = qtgui
    functions_module = types.ModuleType("pyqtgraph.functions")
    functions_module.clip_scalar = clip_scalar
    sys.modules["pyqtgraph"] = package
    sys.modules["pyqtgraph.Qt"] = qt_module
    sys.modules["pyqtgraph.functions"] = functions_module


def load_upstream_classes(reference_root: Path) -> tuple[type[Any], type[Any]]:
    point_path = reference_root / "pyqtgraph" / "Point.py"
    vector_path = reference_root / "pyqtgraph" / "Vector.py"
    missing = [path for path in (point_path, vector_path) if not path.exists()]
    if missing:
        raise SystemExit("Missing pinned PyQtGraph source: " + ", ".join(str(path) for path in missing))

    install_fake_pyqtgraph_package()
    for module_name, source_path in (
        ("pyqtgraph.Point", point_path),
        ("pyqtgraph.Vector", vector_path),
    ):
        spec = importlib.util.spec_from_file_location(module_name, source_path)
        if spec is None or spec.loader is None:
            raise SystemExit(f"Unable to load {source_path}")
        module = importlib.util.module_from_spec(spec)
        module.__package__ = "pyqtgraph"
        sys.modules[module_name] = module
        spec.loader.exec_module(module)

    return sys.modules["pyqtgraph.Point"].Point, sys.modules["pyqtgraph.Vector"].Vector


def point_tuple(point: Any) -> list[float]:
    return [float(point.x()), float(point.y())]


def vector_tuple(vector: Any) -> list[float]:
    return [float(vector.x()), float(vector.y()), float(vector.z())]


def exception_name(callable_: Callable[[], Any]) -> str:
    try:
        callable_()
    except Exception as exc:  # noqa: BLE001 - the fixture records upstream exception class names.
        return type(exc).__name__
    raise AssertionError("expected exception")


def finite_or_nan(values: list[float]) -> list[str | float]:
    return ["nan" if math.isnan(value) else value for value in values]


def build_fixture_from_source(reference_root: Path) -> dict[str, Any]:
    Point, Vector = load_upstream_classes(reference_root)

    point = Point(6.0, 8.0)
    other = Point(2.0, 4.0)
    indexed_point = Point(10.0, 20.0)
    indexed_point[0] = -1.0
    indexed_point[1] = -2.0
    copied_point = Point(3.0, 4.0).copy()
    copied_point[0] = 30.0

    vector = Vector(10.0, 20.0, 30.0)
    vector[0] = -1.0
    vector[1] = -2.0
    vector[2] = -3.0

    return {
        "schema_version": 1,
        "issue": "P2.01",
        "upstream": {
            "project": "pyqtgraph",
            "ref": PINNED_REF,
            "commit": PINNED_COMMIT,
            "files": UPSTREAM_FILES,
        },
        "tolerances": {
            "point_absolute": 1.0e-12,
            "vector_absolute": 1.0e-5,
            "vector_reason": "PyQtGraph Vector is backed by Qt QVector3D float coordinates in both Python and C++.",
        },
        "point": {
            "construction": {
                "default": point_tuple(Point()),
                "two_numeric": point_tuple(Point(1.5, -2.25)),
                "scalar": point_tuple(Point(3.0)),
                "sequence_pair": point_tuple(Point([8.0, 9.0])),
                "qpointf": point_tuple(Point(QPointF(4.5, -5.5))),
                "qpoint": point_tuple(Point(QPoint(4, -5))),
                "qsizef": point_tuple(Point(QSizeF(6.5, 7.5))),
                "qsize": point_tuple(Point(QSize(6, 7))),
                "long_sequence_uses_first_two_python_behavior": point_tuple(Point([1.0, 2.0, 3.0])),
            },
            "invalid_sequence_exceptions": {
                "empty_sequence": exception_name(lambda: Point([])),
                "single_item_sequence": exception_name(lambda: Point([1.0])),
            },
            "coordinate_access": {
                "len": len(Point(1.0, 2.0)),
                "at_0_1": [Point(10.0, 20.0)[0], Point(10.0, 20.0)[1]],
                "after_mutation": point_tuple(indexed_point),
                "iteration": list(Point(10.0, 20.0)),
                "invalid_read_exception": exception_name(lambda: Point(1.0, 2.0)[2]),
                "invalid_write_exception": exception_name(lambda: Point(1.0, 2.0).__setitem__(2, 3.0)),
            },
            "operators": {
                "add_point": point_tuple(point + other),
                "sub_point": point_tuple(point - other),
                "mul_point": point_tuple(point * other),
                "div_point": point_tuple(point / other),
                "add_scalar": point_tuple(point + 2.0),
                "sub_scalar": point_tuple(point - 2.0),
                "mul_scalar": point_tuple(point * 2.0),
                "div_scalar": point_tuple(point / 2.0),
                "radd_scalar": point_tuple(2.0 + point),
                "rsub_scalar": point_tuple(20.0 - point),
                "rmul_scalar": point_tuple(2.0 * point),
                "rdiv_scalar": point_tuple(24.0 / point),
            },
            "geometry": {
                "length_6_8": point.length(),
                "norm_6_8": point_tuple(point.norm()),
                "zero_norm_exception": exception_name(lambda: Point(0.0, 0.0).norm()),
                "angle_degrees": Point(1.0, 0.0).angle(Point(0.0, 1.0)),
                "angle_radians": Point(1.0, 0.0).angle(Point(0.0, 1.0), units="radians"),
                "dot": Point(3.0, 4.0).dot(Point(5.0, 6.0)),
                "cross": Point(3.0, 4.0).cross(Point(5.0, 6.0)),
                "projection_x_axis": point_tuple(Point(3.0, 4.0).proj(Point(10.0, 0.0))),
                "projection_zero_exception": exception_name(lambda: Point(3.0, 4.0).proj(Point(0.0, 0.0))),
                "min": Point(3.0, -4.0).min(),
                "max": Point(3.0, -4.0).max(),
            },
            "copy_to_qpoint": {
                "copy_is_independent_original": point_tuple(Point(3.0, 4.0)),
                "copy_after_mutating_copy": point_tuple(copied_point),
                "to_qpoint": point_tuple(Point(3.0, 4.0).toQPoint()),
            },
        },
        "vector": {
            "construction": {
                "default": vector_tuple(Vector()),
                "two_numeric": vector_tuple(Vector(1.5, -2.25)),
                "three_numeric": vector_tuple(Vector(1.5, -2.25, 3.75)),
                "sequence_pair": vector_tuple(Vector([8.0, 9.0])),
                "sequence_three": vector_tuple(Vector([8.0, 9.0, 10.0])),
                "qpointf": vector_tuple(Vector(QPointF(4.5, -5.5))),
                "qpoint": vector_tuple(Vector(QPoint(4, -5))),
                "qsizef": vector_tuple(Vector(QSizeF(6.5, 7.5))),
                "qvector3d": vector_tuple(Vector(QVector3D(1.0, 2.0, 3.0))),
            },
            "invalid_sequence_exceptions": {
                "single_item_sequence": exception_name(lambda: Vector([1.0])),
                "four_item_sequence": exception_name(lambda: Vector([1.0, 2.0, 3.0, 4.0])),
                "four_constructor_args": exception_name(lambda: Vector(1.0, 2.0, 3.0, 4.0)),
            },
            "coordinate_access": {
                "len": len(Vector(1.0, 2.0, 3.0)),
                "at_0_1_2": [Vector(10.0, 20.0, 30.0)[0], Vector(10.0, 20.0, 30.0)[1], Vector(10.0, 20.0, 30.0)[2]],
                "after_mutation": vector_tuple(vector),
                "iteration": list(Vector(10.0, 20.0, 30.0)),
                "invalid_read_exception": exception_name(lambda: Vector(1.0, 2.0, 3.0)[3]),
                "invalid_write_exception": exception_name(lambda: Vector(1.0, 2.0, 3.0).__setitem__(3, 4.0)),
            },
            "geometry": {
                "angle_90": Vector(1.0, 0.0, 0.0).angle(Vector(0.0, 1.0, 0.0)),
                "angle_45": Vector(1.0, 0.0, 0.0).angle(Vector(1.0, 1.0, 0.0)),
                "angle_parallel": Vector(1.0, 0.0, 0.0).angle(Vector(1.0, 0.0, 0.0)),
                "angle_zero_vector": Vector().angle(Vector(1.0, 0.0, 0.0)),
                "abs": vector_tuple(abs(Vector(-1.0, 2.0, -3.0))),
            },
        },
        "cpp_deviations": [
            "C++ exposes at()/set() and checked Vector operator[] instead of Python __getitem__/__setitem__ syntax for Point and Vector.",
            "C++ Point(std::initializer_list) treats one value as scalar and rejects lists with zero or more than two values; PyQtGraph Point([1, 2, 3]) uses the first two values.",
            "C++ Point::norm() on a zero vector returns NaN coordinates through IEEE floating-point division; PyQtGraph Point.norm() raises ZeroDivisionError.",
            "C++ Point unary negation is retained as an issue-required native coordinate-wise operator; pinned Point.py does not define a Python __neg__ method.",
            "C++ Vector::angle() returns std::nullopt for PyQtGraph's None zero-vector result.",
            "C++ Vector stores QVector3D float coordinates, so vector comparisons use the fixture's vector tolerance.",
            "C++ Point does not currently expose PyQtGraph's Python-only power operators (__pow__/__rpow__).",
        ],
    }


def fallback_fixture() -> dict[str, Any]:
    """Fixture expected when pinned sources are unavailable during --check."""
    return {
        "schema_version": 1,
        "issue": "P2.01",
        "upstream": {
            "project": "pyqtgraph",
            "ref": PINNED_REF,
            "commit": PINNED_COMMIT,
            "files": UPSTREAM_FILES,
        },
        "tolerances": {
            "point_absolute": 1.0e-12,
            "vector_absolute": 1.0e-5,
            "vector_reason": "PyQtGraph Vector is backed by Qt QVector3D float coordinates in both Python and C++.",
        },
        "point": {
            "construction": {
                "default": [0.0, 0.0],
                "two_numeric": [1.5, -2.25],
                "scalar": [3.0, 3.0],
                "sequence_pair": [8.0, 9.0],
                "qpointf": [4.5, -5.5],
                "qpoint": [4.0, -5.0],
                "qsizef": [6.5, 7.5],
                "qsize": [6.0, 7.0],
                "long_sequence_uses_first_two_python_behavior": [1.0, 2.0],
            },
            "invalid_sequence_exceptions": {"empty_sequence": "IndexError", "single_item_sequence": "IndexError"},
            "coordinate_access": {
                "len": 2,
                "at_0_1": [10.0, 20.0],
                "after_mutation": [-1.0, -2.0],
                "iteration": [10.0, 20.0],
                "invalid_read_exception": "IndexError",
                "invalid_write_exception": "IndexError",
            },
            "operators": {
                "add_point": [8.0, 12.0],
                "sub_point": [4.0, 4.0],
                "mul_point": [12.0, 32.0],
                "div_point": [3.0, 2.0],
                "add_scalar": [8.0, 10.0],
                "sub_scalar": [4.0, 6.0],
                "mul_scalar": [12.0, 16.0],
                "div_scalar": [3.0, 4.0],
                "radd_scalar": [8.0, 10.0],
                "rsub_scalar": [14.0, 12.0],
                "rmul_scalar": [12.0, 16.0],
                "rdiv_scalar": [4.0, 3.0],
            },
            "geometry": {
                "length_6_8": 10.0,
                "norm_6_8": [0.6, 0.8],
                "zero_norm_exception": "ZeroDivisionError",
                "angle_degrees": -90.0,
                "angle_radians": -1.5707963267948966,
                "dot": 39.0,
                "cross": -2.0,
                "projection_x_axis": [3.0, 0.0],
                "projection_zero_exception": "ZeroDivisionError",
                "min": -4.0,
                "max": 3.0,
            },
            "copy_to_qpoint": {
                "copy_is_independent_original": [3.0, 4.0],
                "copy_after_mutating_copy": [30.0, 4.0],
                "to_qpoint": [3.0, 4.0],
            },
        },
        "vector": {
            "construction": {
                "default": [0.0, 0.0, 0.0],
                "two_numeric": [1.5, -2.25, 0.0],
                "three_numeric": [1.5, -2.25, 3.75],
                "sequence_pair": [8.0, 9.0, 0.0],
                "sequence_three": [8.0, 9.0, 10.0],
                "qpointf": [4.5, -5.5, 0.0],
                "qpoint": [4.0, -5.0, 0.0],
                "qsizef": [6.5, 7.5, 0.0],
                "qvector3d": [1.0, 2.0, 3.0],
            },
            "invalid_sequence_exceptions": {
                "single_item_sequence": "Exception",
                "four_item_sequence": "Exception",
                "four_constructor_args": "TypeError",
            },
            "coordinate_access": {
                "len": 3,
                "at_0_1_2": [10.0, 20.0, 30.0],
                "after_mutation": [-1.0, -2.0, -3.0],
                "iteration": [10.0, 20.0, 30.0],
                "invalid_read_exception": "IndexError",
                "invalid_write_exception": "IndexError",
            },
            "geometry": {
                "angle_90": 90.0,
                "angle_45": 45.00000000000001,
                "angle_parallel": 0.0,
                "angle_zero_vector": None,
                "abs": [1.0, 2.0, 3.0],
            },
        },
        "cpp_deviations": [
            "C++ exposes at()/set() and checked Vector operator[] instead of Python __getitem__/__setitem__ syntax for Point and Vector.",
            "C++ Point(std::initializer_list) treats one value as scalar and rejects lists with zero or more than two values; PyQtGraph Point([1, 2, 3]) uses the first two values.",
            "C++ Point::norm() on a zero vector returns NaN coordinates through IEEE floating-point division; PyQtGraph Point.norm() raises ZeroDivisionError.",
            "C++ Point unary negation is retained as an issue-required native coordinate-wise operator; pinned Point.py does not define a Python __neg__ method.",
            "C++ Vector::angle() returns std::nullopt for PyQtGraph's None zero-vector result.",
            "C++ Vector stores QVector3D float coordinates, so vector comparisons use the fixture's vector tolerance.",
            "C++ Point does not currently expose PyQtGraph's Python-only power operators (__pow__/__rpow__).",
        ],
    }


def canonical_text(fixture: dict[str, Any]) -> str:
    return json.dumps(fixture, indent=2, sort_keys=True, allow_nan=False) + "\n"


def reference_root_from_args(value: str | None) -> Path | None:
    if value:
        return Path(value)
    env_value = os.environ.get("PGCPP_PYQTGRAPH_REF")
    if env_value:
        return Path(env_value)
    if DEFAULT_REF_ROOT.exists():
        return DEFAULT_REF_ROOT
    return None


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="verify the fixture instead of writing it")
    parser.add_argument("--require-source", action="store_true", help="fail if pinned PyQtGraph sources are unavailable")
    parser.add_argument("--pyqtgraph-root", help="path to the pinned pyqtgraph-0.14.0 checkout")
    args = parser.parse_args()

    reference_root = reference_root_from_args(args.pyqtgraph_root)
    if reference_root is not None:
        fixture = build_fixture_from_source(reference_root)
    elif args.require_source:
        raise SystemExit("Pinned PyQtGraph source is unavailable; pass --pyqtgraph-root or PGCPP_PYQTGRAPH_REF")
    else:
        fixture = fallback_fixture()

    expected = canonical_text(fixture)
    if args.check:
        if not FIXTURE.exists():
            raise SystemExit(f"Missing oracle fixture: {FIXTURE.relative_to(ROOT)}")
        actual = FIXTURE.read_text(encoding="utf-8")
        if actual != expected:
            raise SystemExit(f"Oracle fixture is stale: regenerate {FIXTURE.relative_to(ROOT)}")
        source_note = f" using {reference_root}" if reference_root is not None else " using checked-in fixture metadata"
        print(f"P2.01 oracle fixture OK: {FIXTURE.relative_to(ROOT)}{source_note}")
        return 0

    FIXTURE.parent.mkdir(parents=True, exist_ok=True)
    FIXTURE.write_text(expected, encoding="utf-8")
    print(f"Wrote {FIXTURE.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
