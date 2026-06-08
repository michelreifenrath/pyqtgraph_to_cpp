#!/usr/bin/env python3
"""Standalone P4.07 oracle checker for IsocurveItem contour geometry.

The probe computes the PyQtGraph 0.14.0 IsocurveItem contract without importing
NumPy/Qt, using the marching-squares algorithm in the pinned reference:
- pyqtgraph/graphicsItems/IsocurveItem.py:15-111
- pyqtgraph/functions.py:2313-2492
- pyqtgraph/examples/isocurve.py:27-43
"""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from typing import Iterable

REFERENCE = {
    "source": "pyqtgraph-0.14.0",
    "pinned_commit": "a20028b98294b9cc8770f2015a92eb342224b788",
    "files": [
        "pyqtgraph/graphicsItems/IsocurveItem.py:15-111",
        "pyqtgraph/functions.py:2313-2492",
        "pyqtgraph/examples/isocurve.py:27-43",
    ],
    "note": "Fixture values are computed from PyQtGraph IsocurveItem.generatePath, which transposes row-major data before calling fn.isocurve(data, level, connected=True, extendToEdge=True).",
}

SIDE_TABLE = [
    [],
    [0, 1],
    [1, 2],
    [0, 2],
    [0, 3],
    [1, 3],
    [0, 1, 2, 3],
    [2, 3],
    [2, 3],
    [0, 1, 2, 3],
    [1, 3],
    [0, 3],
    [0, 2],
    [1, 2],
    [0, 1],
    [],
]

EDGE_KEY = [
    ((0, 1), (0, 0)),
    ((0, 0), (1, 0)),
    ((1, 0), (1, 1)),
    ((1, 1), (0, 1)),
]


def transpose(data: list[list[float]]) -> list[list[float]]:
    return [list(row) for row in zip(*data)]


def pad_edge(data: list[list[float]]) -> list[list[float]]:
    rows = len(data)
    cols = len(data[0]) if rows else 0
    out = [[0.0 for _ in range(cols + 2)] for _ in range(rows + 2)]
    for i in range(rows + 2):
        src_i = min(rows - 1, max(0, i - 1))
        for j in range(cols + 2):
            src_j = min(cols - 1, max(0, j - 1))
            out[i][j] = data[src_i][src_j]
    return out


def isocurve(data: list[list[float]], level: float) -> list[list[list[float]]]:
    """PyQtGraph fn.isocurve(data, level, connected=True, extendToEdge=True)."""
    data = pad_edge(data)
    rows = len(data)
    cols = len(data[0]) if rows else 0
    segments = []
    for i in range(rows - 1):
        for j in range(cols - 1):
            index = 0
            for ii in (0, 1):
                for jj in (0, 1):
                    if data[i + ii][j + jj] < level:
                        index += 2 ** (ii + 2 * jj)
            sides = SIDE_TABLE[index]
            for lidx in range(0, len(sides), 2):
                edges = sides[lidx:lidx + 2]
                pts = []
                for edge in edges:
                    p1, p2 = EDGE_KEY[edge]
                    v1 = data[i + p1[0]][j + p1[1]]
                    v2 = data[i + p2[0]][j + p2[1]]
                    f = (level - v1) / (v2 - v1)
                    fi = 1.0 - f
                    x = p1[0] * fi + p2[0] * f + i + 0.5
                    y = p1[1] * fi + p2[1] * f + j + 0.5
                    x = min(rows - 2, max(0.0, x - 1.0))
                    y = min(cols - 2, max(0.0, y - 1.0))
                    grid_key = (i + (1 if edge == 2 else 0), j + (1 if edge == 3 else 0), edge % 2)
                    pts.append(((x, y), grid_key))
                segments.append(pts)

    points: dict[tuple[int, int, int], list[list[tuple[tuple[float, float], tuple[int, int, int]]]]] = {}
    for a, b in segments:
        points.setdefault(a[1], []).append([a, b])
        points.setdefault(b[1], []).append([b, a])

    for k in list(points.keys()):
        try:
            chains = points[k]
        except KeyError:
            continue
        for chain in list(chains):
            x = None
            while True:
                if x == chain[-1][1]:
                    break
                x = chain[-1][1]
                if x == k:
                    break
                y = chain[-2][1]
                connects = points[x]
                for conn in connects[:]:
                    if conn[1][1] != y:
                        chain.extend(conn[1:])
                del points[x]
            if chain[0][1] == chain[-1][1]:
                chains.pop()
                break

    lines = []
    for chain_list in points.values():
        if len(chain_list) == 2:
            chain = list(reversed(chain_list[1][1:])) + chain_list[0]
        else:
            chain = chain_list[0]
        lines.append([[float(p[0][0]), float(p[0][1])] for p in chain])
    return lines


def bounds(lines: list[list[list[float]]]) -> list[float]:
    pts = [point for line in lines for point in line]
    if not pts:
        return [0.0, 0.0, 0.0, 0.0]
    xs = [point[0] for point in pts]
    ys = [point[1] for point in pts]
    return [min(xs), min(ys), max(xs), max(ys)]


def case(name: str, data: list[list[float]], level: float, axis_order: str = "col-major") -> dict[str, object]:
    marching_data = transpose(data) if axis_order == "row-major" else data
    lines = isocurve(marching_data, level)
    return {
        "name": name,
        "axis_order": axis_order,
        "level": level,
        "data": data,
        "shape": [len(data), len(data[0]) if data else 0],
        "lines": lines,
        "bounds": bounds(lines),
    }


def normalize(value):
    if isinstance(value, dict):
        return {k: normalize(v) for k, v in value.items()}
    if isinstance(value, list):
        return [normalize(v) for v in value]
    if isinstance(value, float) and value == 0.0:
        return 0.0
    return value


def build_fixture() -> dict[str, object]:
    cases = [
        case("diagonal_plane_col_major", [[0.0, 1.0], [1.0, 2.0]], 1.0),
        case("closed_peak_loop", [[0.0, 0.0, 0.0], [0.0, 2.0, 0.0], [0.0, 0.0, 0.0]], 1.0),
        case("edge_extension", [[2.0, 2.0, 2.0], [2.0, 0.0, 0.0], [2.0, 0.0, 0.0]], 1.0),
        case("ambiguous_saddle", [[0.0, 2.0], [2.0, 0.0]], 1.0),
        case("row_major_transpose", [[0.0, 1.0, 2.0], [2.0, 1.0, 0.0]], 1.0, "row-major"),
    ]
    return {
        "issue": "P4.07",
        "reference": REFERENCE,
        "tolerance": 1e-9,
        "cases": cases,
        "empty_item": {"bounds": [0.0, 0.0, 0.0, 0.0], "lines": []},
        "visual_case": "closed_peak_loop",
        "visual_tolerance": {"changed_pixels": 0, "max_delta": 0},
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixture", default="oracle/fixtures/P4_07/isocurve_oracle.json")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    fixture = Path(args.fixture)
    expected = build_fixture()
    if args.check:
        observed = json.loads(fixture.read_text(encoding="utf-8"))
        if normalize(observed) != normalize(expected):
            print(json.dumps(expected, indent=2, sort_keys=True))
            raise SystemExit(f"{fixture} does not match P4.07 oracle")
        print(f"P4.07 oracle fixture OK: {fixture}")
        return 0
    fixture.parent.mkdir(parents=True, exist_ok=True)
    fixture.write_text(json.dumps(expected, sort_keys=True, separators=(",", ":")) + "\n", encoding="utf-8")
    print(f"wrote {fixture}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
