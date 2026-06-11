#!/usr/bin/env python3
"""Pinned Plotting p5 AxisItem oracle fixture checker for issue #388."""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

EXPECTED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"
EXPECTED_SOURCE = "pyqtgraph-0.14.0"


def fixture_path() -> Path:
    return Path(__file__).resolve().parents[1] / "fixtures" / "P388" / "axis_p5_oracle.json"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="validate the pinned fixture")
    args = parser.parse_args()

    path = fixture_path()
    data = json.loads(path.read_text(encoding="utf-8"))
    source = data.get("source", {})
    assert source.get("id") == EXPECTED_SOURCE
    assert source.get("pinned_commit") == EXPECTED_COMMIT

    bottom = data["bottom_log_ticks"]
    assert bottom["major_values"] == [-7.0, -6.0, -5.0]
    assert bottom["major_strings"] == ["0.1", "1", "10¹"]
    assert math.isclose(bottom["minor_contains"], -6.0 + math.log10(2.0), rel_tol=0.0, abs_tol=1e-12)

    left = data["left_linear_ticks"]
    assert left["major_values"] == [1.03, 1.04, 1.05, 1.06]
    assert left["major_strings"] == ["1.03", "1.04", "1.05", "1.06"]

    si = data["si_prefix"]
    assert si["bottom"]["prefix"] == "µ"
    assert math.isclose(si["bottom"]["scale"], 1.0e6)
    assert si["left"]["prefix"] == ""
    assert math.isclose(si["left"]["scale"], 1.0)

    print(f"P388 Plotting p5 axis oracle fixture ok: {EXPECTED_SOURCE} {EXPECTED_COMMIT} ({path})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
