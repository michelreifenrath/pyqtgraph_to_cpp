#!/usr/bin/env python3
"""Pinned DateAxisItem oracle fixture checker for P3.08.

The fixture values are generated from the documented PyQtGraph 0.14.0 DateAxisItem
behavior in pyqtgraph/graphicsItems/DateAxisItem.py and checked against upstream
coverage points from tests/graphicsItems/test_DateAxisItem.py. This probe is
intentionally dependency-light for local CTest runs where PyQtGraph's NumPy/Qt
runtime dependencies are not installed.
"""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

EXPECTED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"
EXPECTED_SOURCE = "pyqtgraph-0.14.0"
REQUIRED_CASES = {
    "basic_utc_tick_strings",
    "millisecond_labels",
    "custom_utc_offset_hour_labels",
    "day_hour_ticks_utc",
    "fallback_year_for_out_of_range",
}


def fixture_path() -> Path:
    return Path(__file__).resolve().parents[1] / "fixtures" / "P3_08" / "dateaxis_oracle.json"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="validate the pinned fixture")
    args = parser.parse_args()

    path = fixture_path()
    data = json.loads(path.read_text(encoding="utf-8"))
    source = data.get("source", {})
    assert source.get("id") == EXPECTED_SOURCE
    assert source.get("pinned_commit") == EXPECTED_COMMIT
    upstream_files = set(source.get("upstream_files", []))
    assert "pyqtgraph/graphicsItems/DateAxisItem.py" in upstream_files
    assert "tests/graphicsItems/test_DateAxisItem.py" in upstream_files

    cases = {case["name"]: case for case in data.get("cases", [])}
    missing = REQUIRED_CASES.difference(cases)
    assert not missing, f"missing DateAxisItem oracle cases: {sorted(missing)}"
    assert data["tolerances"]["tick_position_seconds"] <= 1e-9
    assert cases["millisecond_labels"]["expected"][-1] == "59.999"
    assert cases["custom_utc_offset_hour_labels"]["expected"] == ["01:00", "02:00", "03:00", "04:00", "05:00"]
    assert cases["day_hour_ticks_utc"]["expected_levels"][1]["contains"] == [978328800, 978350400, 978372000]
    assert all(math.isfinite(value) for value in cases["basic_utc_tick_strings"]["values"])

    print(
        "P3.08 DateAxisItem oracle fixture ok: "
        f"{EXPECTED_SOURCE} {EXPECTED_COMMIT} ({path})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
