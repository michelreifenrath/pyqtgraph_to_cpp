#!/usr/bin/env python3
"""Generate/check the P2.04 functions and scatter symbol oracle fixture.

The checked-in fixture records the PyQtGraph 0.14.0 function-helper cases and
ScatterPlotItem symbol contract used by the focused C++ proof.  When a pinned
PyQtGraph checkout is available, the probe also verifies that the expected
upstream files and ScatterPlotItem symbol declarations are present before
emitting the same canonical fixture.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_REF_ROOT = ROOT / "reference" / "pyqtgraph"
FIXTURE = ROOT / "oracle" / "fixtures" / "P2_04" / "functions_symbol_oracle.json"
PINNED_REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"
UPSTREAM_FILES = [
    "pyqtgraph/functions.py",
    "tests/test_functions.py",
    "pyqtgraph/graphicsItems/ScatterPlotItem.py",
]
SYMBOL_NAMES = [
    "o",
    "s",
    "t",
    "t1",
    "t2",
    "t3",
    "d",
    "+",
    "x",
    "p",
    "h",
    "star",
    "|",
    "_",
    "arrow_up",
    "arrow_right",
    "arrow_down",
    "arrow_left",
    "crosshair",
]


def color_cases() -> dict[str, Any]:
    return {
        "named_colors": {
            "r": [255, 0, 0, 255],
            "g": [0, 255, 0, 255],
            "b": [0, 0, 255, 255],
            "c": [0, 255, 255, 255],
            "m": [255, 0, 255, 255],
            "y": [255, 255, 0, 255],
            "k": [0, 0, 0, 255],
            "w": [255, 255, 255, 255],
            "d": [150, 150, 150, 255],
            "l": [200, 200, 200, 255],
            "s": [100, 100, 150, 255],
        },
        "hex_strings": {
            "#89a": [136, 153, 170, 255],
            "#89ab": [136, 153, 170, 187],
            "#4488cc": [68, 136, 204, 255],
            "#4488cc00": [68, 136, 204, 0],
        },
        "numeric": {
            "gray_0_75": [191, 191, 191, 255],
            "rgb": [11, 22, 33, 255],
            "rgba": [11, 22, 33, 44],
            "indexed_0_of_2": [255, 0, 0, 255],
            "indexed_1_of_2": [0, 255, 255, 255],
            "indexed_2_of_2": [255, 0, 0, 255],
        },
        "qcolor_copy": [1, 2, 3, 4],
        "qt_named_colors": {
            "steelblue": [70, 130, 180, 255],
            "lawngreen": [124, 252, 0, 255],
        },
    }


def symbol_cases() -> dict[str, Any]:
    return {
        "source_contract": "pyqtgraph/graphicsItems/ScatterPlotItem.py Symbols OrderedDict, name_list, and coords",
        "names_in_order": SYMBOL_NAMES,
        "unit_rect_bounds": {
            "o": [-0.5, -0.5, 1.0, 1.0],
            "s": [-0.5, -0.5, 1.0, 1.0],
            "d": [-0.4, -0.5, 0.8, 1.0],
            "+": [-0.5, -0.5, 1.0, 1.0],
            "crosshair": [-1.0, -1.0, 2.0, 2.0],
        },
        "path_prefixes": {
            "t": [[-0.5, -0.5], [0.0, 0.5], [0.5, -0.5]],
            "t1": [[-0.5, 0.5], [0.0, -0.5], [0.5, 0.5]],
            "t2": [[-0.5, -0.5], [-0.5, 0.5], [0.5, 0.0]],
            "t3": [[0.5, 0.5], [0.5, -0.5], [-0.5, 0.0]],
            "d": [[0.0, -0.5], [-0.4, 0.0], [0.0, 0.5], [0.4, 0.0]],
            "p": [[0.0, -0.5], [-0.4755, -0.1545], [-0.2939, 0.4045], [0.2939, 0.4045], [0.4755, -0.1545]],
            "h": [[0.433, 0.25], [0.0, 0.5], [-0.433, 0.25], [-0.433, -0.25], [0.0, -0.5], [0.433, -0.25]],
            "star": [[0.0, -0.5], [-0.1123, -0.1545], [-0.4755, -0.1545], [-0.1816, 0.059], [-0.2939, 0.4045], [0.0, 0.191], [0.2939, 0.4045], [0.1816, 0.059], [0.4755, -0.1545], [0.1123, -0.1545]],
            "|": [[-0.1, 0.5], [0.1, 0.5], [0.1, -0.5], [-0.1, -0.5]],
            "_": [[-0.5, -0.1], [-0.5, 0.1], [0.5, 0.1], [0.5, -0.1]],
            "arrow_up": [[-0.125, 0.125], [0.0, 0.0], [0.125, 0.125], [0.05, 0.125], [0.05, 0.5], [-0.05, 0.5], [-0.05, 0.125]],
            "arrow_right": [[-0.125, -0.125], [0.0, 0.0], [-0.125, 0.125], [-0.125, 0.05], [-0.5, 0.05], [-0.5, -0.05], [-0.125, -0.05]],
            "arrow_down": [[0.125, -0.125], [0.0, 0.0], [-0.125, -0.125], [-0.05, -0.125], [-0.05, -0.5], [0.05, -0.5], [0.05, -0.125]],
            "arrow_left": [[0.125, 0.125], [0.0, 0.0], [0.125, -0.125], [0.125, -0.05], [0.5, -0.05], [0.5, 0.05], [0.125, 0.05]],
        },
        "edge_cases": {
            "unknown_symbol": "rejected",
            "null_c_string": "rejected by C++ helper before QString conversion",
        },
    }


def fixture_payload() -> dict[str, Any]:
    return {
        "schema_version": 1,
        "issue": "P2.04",
        "upstream": {
            "project": "pyqtgraph",
            "ref": PINNED_REF,
            "commit": PINNED_COMMIT,
            "files": UPSTREAM_FILES,
            "probe_note": "Color/pen/brush cases come from functions.py and tests/test_functions.py; symbol names and coordinates come from graphicsItems/ScatterPlotItem.py.",
        },
        "tolerances": {
            "qcolor_channels": 0,
            "path_coordinates_absolute": 1.0e-12,
            "path_contains": "Qt QPainterPath boolean containment for representative interior/exterior points",
        },
        "cases": {
            "mkColor": color_cases(),
            "mkPen": {
                "kwargs_equivalent": "PenOptions covers hsv, width, style, dash, and cosmetic flags",
                "none": "Qt::NoPen",
                "wide_line_cap_threshold": 4.0,
            },
            "mkBrush": {
                "none": "Qt::NoBrush",
                "solid_qcolor_copy": [1, 2, 3, 4],
                "array_rgba": [11, 12, 13, 14],
            },
            "symbols": symbol_cases(),
        },
        "cpp_deviations": [
            "C++ exposes symbolPaths()/symbolPath() accessors for the ScatterPlotItem.py Symbols table instead of a mutable Python module-level OrderedDict.",
            "C++ unknown symbols raise std::invalid_argument; Python callers indexing Symbols would receive KeyError.",
        ],
    }


def validate_reference_root(reference_root: Path) -> None:
    missing = [path for path in (reference_root / file for file in UPSTREAM_FILES) if not path.exists()]
    if missing:
        raise SystemExit("Missing pinned PyQtGraph source: " + ", ".join(str(path) for path in missing))

    scatter_text = (reference_root / "pyqtgraph/graphicsItems/ScatterPlotItem.py").read_text(encoding="utf-8")
    required_markers = ["Symbols = OrderedDict", "name_list = [", "coords = {", "crosshair"]
    missing_markers = [marker for marker in required_markers if marker not in scatter_text]
    if missing_markers:
        raise SystemExit("Pinned ScatterPlotItem.py does not contain expected symbol markers: " + ", ".join(missing_markers))


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
        validate_reference_root(reference_root)
    elif args.require_source:
        raise SystemExit("Pinned PyQtGraph source is unavailable; pass --pyqtgraph-root or PGCPP_PYQTGRAPH_REF")

    expected = canonical_text(fixture_payload())
    if args.check:
        if not FIXTURE.exists():
            raise SystemExit(f"Missing oracle fixture: {FIXTURE.relative_to(ROOT)}")
        actual = FIXTURE.read_text(encoding="utf-8")
        if actual != expected:
            raise SystemExit(f"Oracle fixture is stale: regenerate {FIXTURE.relative_to(ROOT)}")
        source_note = f" after validating {reference_root}" if reference_root is not None else " using checked-in fixture metadata"
        print(f"P2.04 oracle fixture OK: {FIXTURE.relative_to(ROOT)}{source_note}")
        return 0

    FIXTURE.parent.mkdir(parents=True, exist_ok=True)
    FIXTURE.write_text(expected, encoding="utf-8")
    print(f"Wrote {FIXTURE.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
