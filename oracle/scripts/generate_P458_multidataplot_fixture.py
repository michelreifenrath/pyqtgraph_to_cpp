#!/usr/bin/env python3
"""Generate deterministic MultiDataPlot fixture data from pinned PyQtGraph."""

from __future__ import annotations

import argparse
import json
import random
import sys
from pathlib import Path
from typing import Any

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
PINNED_CHECKOUT = ROOT / "reference" / "pyqtgraph"
SOURCE_LOCK = ROOT / "reference" / "source.lock"
MULTIDATAPLOT_EXAMPLE = PINNED_CHECKOUT / "pyqtgraph" / "examples" / "MultiDataPlot.py"
DEFAULT_OUTPUT = ROOT / "oracle" / "fixtures" / "P458" / "multidataplot_data.json"
PINNED_REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"
NUMPY_SEED = 10
PYTHON_RANDOM_SEED = 10
RANDOM_SELECTION_COUNT = 4


def require_pinned_sources() -> None:
    missing = [
        path
        for path in (SOURCE_LOCK, MULTIDATAPLOT_EXAMPLE)
        if not path.exists()
    ]
    if missing:
        names = ", ".join(str(path.relative_to(ROOT)) for path in missing)
        raise SystemExit(f"Pinned PyQtGraph checkout is unavailable; missing {names}")

    lock_text = SOURCE_LOCK.read_text(encoding="utf-8")
    if PINNED_COMMIT not in lock_text or PINNED_REF not in lock_text:
        raise SystemExit(
            "reference/source.lock does not match the MultiDataPlot pinned ref/commit"
        )


def sorted_randint(rng: np.random.Generator, low: int, high: int, size: int) -> list[int]:
    return np.sort(rng.integers(low, high, size)).tolist()


def build_multidataplot_fixture() -> dict[str, Any]:
    """Mirror MultiDataPlot.py value generation and random selection order."""
    rng = np.random.default_rng(NUMPY_SEED)
    random.seed(PYTHON_RANDOM_SEED)

    values = {
        "None (replaced by integer indices)": None,
        "Single curve values": sorted_randint(rng, 0, 20, 15),
        "container of (optionally) mixed-size curve values": [
            sorted_randint(rng, 0, 20, 15),
            *[sorted_randint(rng, 0, 20, 15) for _ in range(4)],
        ],
        "2D matrix": [sorted_randint(rng, 20, 40, 15) for _ in range(6)],
    }

    selections: list[dict[str, str]] = []
    for _ in range(RANDOM_SELECTION_COUNT):
        xtype = random.choice(list(values))
        ytype = random.choice(list(values))
        selections.append({"xtype": xtype, "ytype": ytype})

    return {
        "description": (
            "Deterministic MultiDataPlot data arrays from pinned PyQtGraph "
            "examples/MultiDataPlot.py "
            f"(np.random.default_rng({NUMPY_SEED}), random.seed({PYTHON_RANDOM_SEED})). "
            "Used by the C++ test-mode fixture loader; never derived from C++ output."
        ),
        "source": "reference/pyqtgraph/pyqtgraph/examples/MultiDataPlot.py",
        "pyqtgraph_ref": PINNED_REF,
        "pinned_commit": PINNED_COMMIT,
        "numpy_seed": NUMPY_SEED,
        "python_random_seed": PYTHON_RANDOM_SEED,
        "value_keys": list(values.keys()),
        "values": {
            "single_curve_values": values["Single curve values"],
            "container": values["container of (optionally) mixed-size curve values"],
            "matrix_2d": values["2D matrix"],
        },
        "random_selections": selections,
    }


def write_multidataplot_fixture(path: Path = DEFAULT_OUTPUT) -> dict[str, Any]:
    fixture = build_multidataplot_fixture()
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(fixture, indent=2) + "\n", encoding="utf-8")
    return fixture


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Export deterministic MultiDataPlot arrays and random selections "
            "from pinned PyQtGraph seed logic."
        )
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help="Fixture JSON destination",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    require_pinned_sources()
    write_multidataplot_fixture(args.output)
    print(f"wrote MultiDataPlot data fixture: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
