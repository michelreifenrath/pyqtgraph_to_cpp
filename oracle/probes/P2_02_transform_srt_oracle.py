#!/usr/bin/env python3
"""Generate/check the P2.02 Transform3D/SRTTransform oracle fixture.

The fixture records deterministic numeric probes for the pinned PyQtGraph
0.14.0 Transform3D.py, SRTTransform.py, and SRTTransform3D.py behavior.  The
local environment used by the C++ factory does not provide Python Qt/numpy, so
this probe checks the pinned source identity and computes the same no-shear
matrix/map/decomposition cases with pure Python formulas matching the upstream
Qt transform call order.
"""

from __future__ import annotations

import argparse
import json
import math
import os
import subprocess
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_REF_ROOT = ROOT / "reference" / "pyqtgraph"
FIXTURE = ROOT / "oracle" / "fixtures" / "P2_02" / "transform_srt_oracle.json"
PINNED_REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"
UPSTREAM_FILES = [
    "pyqtgraph/Transform3D.py",
    "pyqtgraph/SRTTransform.py",
    "pyqtgraph/SRTTransform3D.py",
    "pyqtgraph/functions.py",
    "tests/test_srttransform3d.py",
    "tests/test_pickles.py",
]


def matmul4(a: list[list[float]], b: list[list[float]]) -> list[list[float]]:
    return [[sum(a[row][k] * b[k][col] for k in range(4)) for col in range(4)] for row in range(4)]


def identity4() -> list[list[float]]:
    return [[1.0 if row == col else 0.0 for col in range(4)] for row in range(4)]


def translate4(x: float, y: float, z: float) -> list[list[float]]:
    m = identity4()
    m[0][3] = x
    m[1][3] = y
    m[2][3] = z
    return m


def scale4(x: float, y: float, z: float) -> list[list[float]]:
    m = identity4()
    m[0][0] = x
    m[1][1] = y
    m[2][2] = z
    return m


def rotate4(angle_degrees: float, axis: tuple[float, float, float]) -> list[list[float]]:
    ax, ay, az = axis
    length = math.sqrt(ax * ax + ay * ay + az * az)
    if length == 0.0:
        raise ValueError("rotation axis must be non-zero")
    ax, ay, az = ax / length, ay / length, az / length
    radians = math.radians(angle_degrees)
    c = math.cos(radians)
    s = math.sin(radians)
    t = 1.0 - c
    return [
        [t * ax * ax + c, t * ax * ay - s * az, t * ax * az + s * ay, 0.0],
        [t * ax * ay + s * az, t * ay * ay + c, t * ay * az - s * ax, 0.0],
        [t * ax * az - s * ay, t * ay * az + s * ax, t * az * az + c, 0.0],
        [0.0, 0.0, 0.0, 1.0],
    ]


def srt3d_matrix(pos: tuple[float, float, float], angle: float, axis: tuple[float, float, float], scale: tuple[float, float, float]) -> list[list[float]]:
    # Upstream SRTTransform3D.update() calls translate, rotate, scale on
    # QMatrix4x4; Qt post-multiplies these modifications.
    return matmul4(matmul4(translate4(*pos), rotate4(angle, axis)), scale4(*scale))


def map3d(matrix: list[list[float]], point: tuple[float, float, float]) -> list[float]:
    x, y, z = point
    return [
        matrix[0][0] * x + matrix[0][1] * y + matrix[0][2] * z + matrix[0][3],
        matrix[1][0] * x + matrix[1][1] * y + matrix[1][2] * z + matrix[1][3],
        matrix[2][0] * x + matrix[2][1] * y + matrix[2][2] * z + matrix[2][3],
    ]


def transform3d_matrix2d(matrix: list[list[float]]) -> list[list[float]]:
    # Transform3D.matrix(nd=2): m[2] = m[3]; m[:,2] = m[:,3]; return m[:3,:3]
    copied = [row[:] for row in matrix]
    copied[2] = copied[3][:]
    for row in range(4):
        copied[row][2] = copied[row][3]
    return [row[:3] for row in copied[:3]]


def srt2d_matrix(pos: tuple[float, float], angle: float, scale: tuple[float, float]) -> list[list[float]]:
    c = math.cos(math.radians(angle))
    s = math.sin(math.radians(angle))
    sx, sy = scale
    tx, ty = pos
    # Upstream SRTTransform.matrix() returns [[m11,m12,m13], ...] from QTransform.
    return [
        [c * sx, s * sx, 0.0],
        [-s * sy, c * sy, 0.0],
        [tx, ty, 1.0],
    ]


def map2d(matrix: list[list[float]], point: tuple[float, float]) -> list[float]:
    x, y = point
    return [
        matrix[0][0] * x + matrix[1][0] * y + matrix[2][0],
        matrix[0][1] * x + matrix[1][1] * y + matrix[2][1],
    ]


def flatten(matrix: list[list[float]]) -> list[float]:
    return [value for row in matrix for value in row]


def verify_reference_root(reference_root: Path, require_source: bool) -> dict[str, Any]:
    missing = [rel for rel in UPSTREAM_FILES if not (reference_root / rel).exists()]
    version_file = reference_root / "pyqtgraph" / "__init__.py"
    version_ok = version_file.exists() and "__version__ = '0.14.0'" in version_file.read_text(encoding="utf-8")
    commit = None
    if (reference_root / ".git").exists():
        commit = subprocess.check_output(["git", "-C", str(reference_root), "rev-parse", "HEAD"], text=True).strip()
    commit_ok = commit in (None, PINNED_COMMIT)
    if require_source and (missing or not version_ok or not commit_ok):
        raise SystemExit(
            "Missing or mismatched pinned PyQtGraph source: "
            + json.dumps({"root": str(reference_root), "missing": missing, "version_ok": version_ok, "commit": commit})
        )
    return {"root": str(reference_root), "missing": missing, "version_ok": version_ok, "commit": commit}


def build_fixture(reference_root: Path, require_source: bool) -> dict[str, Any]:
    verify_reference_root(reference_root, require_source)
    matrix3d = srt3d_matrix((10.0, 20.0, 40.0), 45.0, (0.0, 0.0, 1.0), (0.2, 0.4, 1.0))
    matrix2d = srt2d_matrix((10.0, 20.0), 45.0, (0.2, 0.4))
    identity = identity4()
    return {
        "schema_version": 1,
        "issue": "P2.02",
        "reference": {
            "project": "PyQtGraph",
            "ref": PINNED_REF,
            "commit": PINNED_COMMIT,
            "upstream_files": UPSTREAM_FILES,
        },
        "tolerances": {
            "matrix_absolute": 1e-5,
            "point_absolute": 1e-5,
            "vector_absolute": 1e-5,
        },
        "transform3d": {
            "identity_matrix3d": identity,
            "identity_matrix2d": transform3d_matrix2d(identity),
            "srt_z_matrix3d": matrix3d,
            "srt_z_matrix2d_projection": transform3d_matrix2d(matrix3d),
            "srt_z_flat_row_major": flatten(matrix3d),
            "map_vector": {
                "input": [1.0, 0.0, 0.0],
                "output": map3d(matrix3d, (1.0, 0.0, 0.0)),
            },
            "invalid_sequence_length_exception": "TypeError",
        },
        "srttransform": {
            "state": {"pos": [10.0, 20.0], "scale": [0.2, 0.4], "angle": 45.0},
            "matrix": matrix2d,
            "map_points": [
                {"input": [0.0, 0.0], "output": map2d(matrix2d, (0.0, 0.0))},
                {"input": [1.0, 0.0], "output": map2d(matrix2d, (1.0, 0.0))},
                {"input": [0.0, 1.0], "output": map2d(matrix2d, (0.0, 1.0))},
                {"input": [-2.0, 3.0], "output": map2d(matrix2d, (-2.0, 3.0))},
            ],
            "division_semantics": "A / B == B^-1 * A",
        },
        "srttransform3d": {
            "state": {"pos": [10.0, 20.0, 40.0], "scale": [0.2, 0.4, 1.0], "angle": 45.0, "axis": [0.0, 0.0, 1.0]},
            "matrix": matrix3d,
            "map_points": [
                {"input": [0.0, 0.0, 0.0], "output": map3d(matrix3d, (0.0, 0.0, 0.0))},
                {"input": [1.0, 0.0, 0.0], "output": map3d(matrix3d, (1.0, 0.0, 0.0))},
                {"input": [0.0, 1.0, 0.0], "output": map3d(matrix3d, (0.0, 1.0, 0.0))},
                {"input": [0.0, 0.0, 1.0], "output": map3d(matrix3d, (0.0, 0.0, 1.0))},
                {"input": [-1.0, -1.0, 0.0], "output": map3d(matrix3d, (-1.0, -1.0, 0.0))},
            ],
            "round_trip_from_transform3d": {
                "angle": 45.0,
                "axis": [0.0, 0.0, 1.0],
                "scale": [0.2, 0.4, 1.0],
                "translation": [10.0, 20.0, 40.0],
            },
            "two_value_scale_defaults_z": [2.0, 3.0, 1.0],
            "zero_angle_axis_reset": [0.0, 0.0, 1.0],
            "non_z_as2d_exception": "invalid_argument",
        },
        "cpp_deviations": [
            "C++ exposes matrix3D()/matrix2D() fixed-size arrays and matrix(nd) vector output instead of numpy arrays.",
            "C++ saveState()/restoreState() use typed state structs instead of Python dict/pickle protocols.",
            "C++ map(std::array)/map(std::vector) are native equivalents for PyQtGraph list/tuple/numpy coordinate mapping.",
        ],
    }


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="verify the checked-in fixture")
    parser.add_argument("--write", action="store_true", help="write the fixture")
    parser.add_argument("--require-source", action="store_true", help="fail if the pinned PyQtGraph files are absent")
    parser.add_argument("--pyqtgraph-root", type=Path, default=None)
    args = parser.parse_args(argv)

    reference_root = args.pyqtgraph_root or Path(os.environ.get("PGCPP_PYQTGRAPH_REF", DEFAULT_REF_ROOT))
    fixture = build_fixture(reference_root, args.require_source)

    if args.write:
        FIXTURE.parent.mkdir(parents=True, exist_ok=True)
        FIXTURE.write_text(json.dumps(fixture, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    if args.check:
        if not FIXTURE.exists():
            raise SystemExit(f"missing fixture: {FIXTURE}")
        current = json.loads(FIXTURE.read_text(encoding="utf-8"))
        if current != fixture:
            raise SystemExit("P2.02 oracle fixture is stale; rerun with --write")
        print(f"P2.02 oracle fixture OK: {FIXTURE.relative_to(ROOT)}")

    if not args.check and not args.write:
        print(json.dumps(fixture, indent=2, sort_keys=True))

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
