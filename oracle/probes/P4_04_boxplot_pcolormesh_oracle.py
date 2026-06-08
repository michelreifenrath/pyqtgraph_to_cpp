#!/usr/bin/env python3
"""Standalone P4.04 oracle checker for BoxplotItem/PColorMeshItem fixture.

The local environment for this factory run may not have NumPy/Qt Python wheels.
This probe therefore computes the pinned PyQtGraph 0.14.0 externally visible
algorithms directly from the upstream source contract rather than importing Qt:
- pyqtgraph/graphicsItems/BoxplotItem.py:15-280 (linear percentile/IQR whiskers/bounds)
- pyqtgraph/graphicsItems/PColorMeshItem.py:167-454 (grid synthesis, bounds, polygon order, LUT index normalization)
  with the issue contract's z-only row/column axis parity: x spans columns and y spans rows.
- pyqtgraph/functions.py:1303-1354 (rescaleData clip-before-int conversion)
"""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

REFERENCE = {
    "source": "pyqtgraph-0.14.0",
    "pinned_commit": "a20028b98294b9cc8770f2015a92eb342224b788",
    "files": [
        "pyqtgraph/graphicsItems/BoxplotItem.py:15-280",
        "pyqtgraph/graphicsItems/PColorMeshItem.py:71-454",
        "pyqtgraph/functions.py:1303-1354",
    ],
    "note": "Fixture values are computed by a standalone oracle probe from the pinned PyQtGraph 0.14.0 algorithms: NumPy linear percentile/IQR whiskers for BoxplotItem and PColorMeshItem bounds, finite-cell skipping, polygon order, and integer LUT-index normalization. The issue contract's z-only mesh parity keeps x on columns and y on rows for non-square arrays.",
}


def percentile(values: list[float], pct: float) -> float:
    ordered = sorted(values)
    if len(ordered) == 1:
        return ordered[0]
    rank = (len(ordered) - 1) * pct / 100.0
    lo = math.floor(rank)
    hi = math.ceil(rank)
    if lo == hi:
        return ordered[lo]
    return ordered[lo] + (ordered[hi] - ordered[lo]) * (rank - lo)


def iqr_1p5(values: list[float]) -> tuple[float, float]:
    p75 = percentile(values, 75)
    p25 = percentile(values, 25)
    iqr = p75 - p25
    upper_theory = p75 + 1.5 * iqr
    lower_theory = p25 - 1.5 * iqr
    upper = max(value for value in values if value <= upper_theory)
    lower = min(value for value in values if value >= lower_theory)
    return lower, upper


def box_stats(data: list[list[float]], loc: list[float], whisker=iqr_1p5) -> list[dict[str, object]]:
    rows = []
    for pos, values in zip(loc, data):
        lower, upper = whisker(values)
        rows.append({
            "loc": pos,
            "p25": percentile(values, 25),
            "median": percentile(values, 50),
            "p75": percentile(values, 75),
            "lower": lower,
            "upper": upper,
            "outliers": [value for value in values if value < lower or value > upper],
        })
    return rows


def pcolor_index(value: float, low: float, high: float, lut_rows: int) -> int:
    rng = high - low
    if rng == 0:
        rng = 1
    scaled = (value - low) * ((lut_rows - 1) / rng)
    clipped = min(max(scaled, 0), lut_rows - 1)
    return math.trunc(clipped)


def build_fixture() -> dict[str, object]:
    data = [[1, 2, 3, 4, 100], [-5, 0, 5]]
    loc = [0, 1]
    return {
        "issue": "P4.04",
        "reference": REFERENCE,
        "tolerance": 1e-9,
        "boxplot": {
            "default_case": {
                "data": data,
                "loc": loc,
                "width": 0.8,
                "loc_as_x": True,
                "stats": box_stats(data, loc),
                "bounds_with_outliers": {"x": [-0.4, 1.4], "y": [-5, 100]},
                "bounds_without_outliers": {"x": [-0.4, 1.4], "y": [-5, 5]},
                "bounds_horizontal_with_outliers": {"x": [-5, 100], "y": [-0.4, 1.4]},
            },
            "custom_loc_case": {"loc": [10, 20], "bounds_with_outliers": {"x": [9.6, 20.4], "y": [-5, 100]}},
            "custom_minmax_whisker": {
                "stats": box_stats(data, loc, whisker=lambda values: (min(values), max(values))),
                "bounds_without_outliers": {"x": [-0.4, 1.4], "y": [-5, 100]},
            },
            "hidden_box_width": 0,
        },
        "pcolormesh": {
            "z_only": {
                "z": [[0, 1, 2], ["nan", 4, 4]],
                "shape": [2, 3],
                "bounds": {"x": [0, 3], "y": [0, 2]},
                "levels": [0, 4],
                "lut_rows": 5,
                "cells": [
                    {"row": 0, "col": 0, "value": 0, "color_index": pcolor_index(0, 0, 4, 5), "polygon": [[0, 0], [0, 1], [1, 1], [1, 0]]},
                    {"row": 0, "col": 1, "value": 1, "color_index": pcolor_index(1, 0, 4, 5), "polygon": [[1, 0], [1, 1], [2, 1], [2, 0]]},
                    {"row": 0, "col": 2, "value": 2, "color_index": pcolor_index(2, 0, 4, 5), "polygon": [[2, 0], [2, 1], [3, 1], [3, 0]]},
                    {"row": 1, "col": 1, "value": 4, "color_index": pcolor_index(4, 0, 4, 5), "polygon": [[1, 1], [1, 2], [2, 2], [2, 1]]},
                    {"row": 1, "col": 2, "value": 4, "color_index": pcolor_index(4, 0, 4, 5), "polygon": [[2, 1], [2, 2], [3, 2], [3, 1]]},
                ],
                "nan_cell": {"row": 1, "col": 0, "skipped": True},
                "equal_levels_indices": [pcolor_index(2, 2, 2, 5), pcolor_index(4, 2, 2, 5)],
            },
            "explicit_xy": {
                "x": [[0, 0.5, 1.5], [2, 2.5, 3.5], [4, 4.5, 6]],
                "y": [[0, 1, 2], [0.2, 1.3, 2.6], [0.4, 1.6, 3.0]],
                "z": [[10, 20], [30, 40]],
                "bounds": {"x": [0, 6], "y": [0, 3]},
                "levels": [10, 40],
                "lut_rows": 4,
                "color_indices": [pcolor_index(v, 10, 40, 4) for v in [10, 20, 30, 40]],
                "cell_0_1_polygon": [[0.5, 1], [2.5, 1.3], [3.5, 2.6], [1.5, 2]],
                "shape_mismatch_raises": True,
            },
        },
    }


def normalize(value):
    if isinstance(value, dict):
        return {k: normalize(v) for k, v in value.items()}
    if isinstance(value, list):
        return [normalize(v) for v in value]
    return value


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixture", default="oracle/fixtures/P4_04/boxplot_pcolormesh_oracle.json")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    fixture = Path(args.fixture)
    expected = build_fixture()
    if args.check:
        observed = json.loads(fixture.read_text(encoding="utf-8"))
        if normalize(observed) != normalize(expected):
            print(json.dumps(expected, indent=2, sort_keys=True))
            raise SystemExit(f"{fixture} does not match P4.04 oracle")
        print(f"P4.04 oracle fixture OK: {fixture}")
        return 0
    fixture.parent.mkdir(parents=True, exist_ok=True)
    fixture.write_text(json.dumps(expected, sort_keys=True, separators=(",", ":")) + "\n", encoding="utf-8")
    print(f"wrote {fixture}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
