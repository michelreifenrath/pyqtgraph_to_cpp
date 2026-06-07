#!/usr/bin/env python3
"""Generate/check the P2.06 ColorMap palette/LUT/gradient oracle fixture.

The fixture records representative PyQtGraph 0.14.0 behavior from
``pyqtgraph/colormap.py``, ``pyqtgraph/colors/palette.py``, and
``tests/test_colormap.py``.  When a pinned PyQtGraph checkout is available, the
probe verifies those upstream files are present and still contain the behavior
markers used by this focused C++ proof.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_REF_ROOT = ROOT / "reference" / "pyqtgraph"
FIXTURE = ROOT / "oracle" / "fixtures" / "P2_06" / "colormap_oracle.json"
PINNED_REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"
UPSTREAM_FILES = [
    "pyqtgraph/colormap.py",
    "pyqtgraph/colors/palette.py",
    "tests/test_colormap.py",
    "pyqtgraph/colors/maps/PAL-relaxed.hex",
    "pyqtgraph/colors/maps/PAL-relaxed_bright.hex",
]


def fixture_payload() -> dict[str, Any]:
    return {
        "schema_version": 1,
        "issue": "P2.06",
        "upstream": {
            "project": "pyqtgraph",
            "ref": PINNED_REF,
            "commit": PINNED_COMMIT,
            "files": UPSTREAM_FILES,
            "probe_note": (
                "ColorMap equal spacing, getStops/getColors/getByIndex, map/getLookupTable, "
                "getGradient, listMaps/get, and QDarkStyle palette behavior are taken from "
                "pyqtgraph/colormap.py, pyqtgraph/colors/palette.py, tests/test_colormap.py, "
                "and representative local map files."
            ),
        },
        "tolerances": {
            "byte_channels": 0,
            "float_channels_absolute": 1.0e-6,
            "positions_absolute": 1.0e-12,
            "qcolor_channels": 0,
        },
        "cases": {
            "equal_spacing": {
                "input_colors_rgba": [[0, 0, 0, 255], [128, 64, 32, 255], [255, 255, 255, 255]],
                "positions": [0.0, 0.5, 1.0],
                "lookup_5_byte_rgb": [
                    [0, 0, 0],
                    [64, 32, 16],
                    [128, 64, 32],
                    [191, 159, 143],
                    [255, 255, 255],
                ],
                "lookup_5_float_red": [0.0, 64.0 / 255.0, 128.0 / 255.0, 191.5 / 255.0, 1.0],
            },
            "mapping_modes": {
                "clip_mid_gray_byte": [127, 127, 127, 255],
                "repeat_samples_red": {"-0.25": 191, "1.25": 63},
                "mirror_samples_red": {"-0.25": 63, "1.25": 255},
                "diverging_samples_red": {"-1.0": 0, "0.0": 127, "1.0": 255},
            },
            "gradient": {
                "normal_stops": [[0.0, [0, 0, 0, 255]], [1.0, [255, 255, 255, 255]]],
                "mirror_stops": [
                    [0.0, [255, 255, 255, 255]],
                    [0.5, [0, 0, 0, 255]],
                    [1.0, [255, 255, 255, 255]],
                ],
                "repeat_spread": "QGradient::RepeatSpread",
            },
            "local_palettes": {
                "listMaps_contains": ["PAL-relaxed", "PAL-relaxed_bright"],
                "PAL-relaxed": {
                    "positions": [0.0, 1.0 / 9.0, 2.0 / 9.0, 3.0 / 9.0, 4.0 / 9.0, 5.0 / 9.0, 6.0 / 9.0, 7.0 / 9.0, 8.0 / 9.0, 1.0],
                    "colors_hex": [
                        "#f97f10",
                        "#e5bb00",
                        "#94ab00",
                        "#12a12a",
                        "#007c8c",
                        "#0067d0",
                        "#a02fdb",
                        "#c01188",
                        "#e23512",
                        "#f97f10",
                    ],
                },
                "PAL-relaxed_bright": {
                    "colors_hex": [
                        "#ff9d47",
                        "#f7e100",
                        "#b3cf00",
                        "#1ec23a",
                        "#00a0b5",
                        "#1683f0",
                        "#c56bff",
                        "#e22ca8",
                        "#ff532b",
                        "#ff9d47",
                    ],
                },
            },
            "qdarkstyle_palettes": {
                "dark": {
                    "Active/Base": "#19232D",
                    "Active/ButtonText": "#F0F0F0",
                    "Disabled/Text": "#9DA9B5",
                },
                "light": {
                    "Active/Base": "#FAFAFA",
                    "Active/Highlight": "#9FCBFF",
                    "Disabled/HighlightedText": "#293544",
                },
            },
        },
        "cpp_deviations": [
            "C++ listMaps()/get() intentionally expose the representative local palette shard owned by P2.06 rather than vendoring every upstream colors/maps data file.",
            "C++ ColorMap methods use typed overloads/enums instead of Python string mode dispatch.",
        ],
    }


def validate_reference_root(reference_root: Path) -> None:
    missing = [path for path in (reference_root / file for file in UPSTREAM_FILES) if not path.exists()]
    if missing:
        raise SystemExit("Missing pinned PyQtGraph source: " + ", ".join(str(path) for path in missing))

    markers = {
        "pyqtgraph/colormap.py": [
            "def listMaps(source=None):",
            "def get(name, source=None, skipCache=False):",
            "if pos is None:",
            "def getGradient(self, p1=None, p2=None):",
            "def getLookupTable(self, start=0.0, stop=1.0, nPts=512, alpha=None, mode=BYTE):",
        ],
        "pyqtgraph/colors/palette.py": [
            "def getQDarkStyleDarkQPalette():",
            "def getQDarkStyleLightQPalette():",
            "#19232D",
            "#FAFAFA",
        ],
        "tests/test_colormap.py": [
            "test_ColorMap_getStops",
            "test_ColorMap_getColors",
            "test_ColorMap_getByIndex",
            "test_round_trip",
        ],
        "pyqtgraph/colors/maps/PAL-relaxed.hex": ["#f97f10", "#c01188"],
        "pyqtgraph/colors/maps/PAL-relaxed_bright.hex": ["#ff9d47", "#e22ca8"],
    }
    for rel_path, required in markers.items():
        text = (reference_root / rel_path).read_text(encoding="utf-8")
        absent = [marker for marker in required if marker not in text]
        if absent:
            raise SystemExit(f"Pinned {rel_path} missing expected markers: {', '.join(absent)}")


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
        print(f"P2.06 oracle fixture OK: {FIXTURE.relative_to(ROOT)}{source_note}")
        return 0

    FIXTURE.parent.mkdir(parents=True, exist_ok=True)
    FIXTURE.write_text(expected, encoding="utf-8")
    print(f"Wrote {FIXTURE.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
