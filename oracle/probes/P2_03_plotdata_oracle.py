#!/usr/bin/env python3
"""Generate/check the P2.03 PlotData normalization oracle fixture.

The probe executes pinned PyQtGraph 0.14.0 ``pyqtgraph/PlotData.py``
against a minimal NumPy stand-in for the specific reductions needed here.  This
keeps the issue proof local while still verifying that the public PlotData class
continues to delegate min/max behavior through ``np.min`` / ``np.max``.
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
from typing import Any, Callable, Iterable

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_REF_ROOT = ROOT / "reference" / "pyqtgraph"
FIXTURE = ROOT / "oracle" / "fixtures" / "P2_03" / "plotdata_oracle.json"
PINNED_REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"
UPSTREAM_FILES = [
    "pyqtgraph/PlotData.py",
    "tests/graphicsItems/test_PlotDataItem.py",
    "pyqtgraph/graphicsItems/PlotDataItem.py",
]


class MaskedScalar:
    def __repr__(self) -> str:
        return "masked"


MASKED = MaskedScalar()


class MaskedArray:
    def __init__(self, values: Iterable[Any], mask: Iterable[Any]) -> None:
        self.values = [float(value) for value in values]
        self.mask = [bool(value) for value in mask]
        if len(self.values) != len(self.mask):
            raise ValueError("mask and values must have the same length")

    def unmasked_values(self) -> list[float]:
        return [value for value, masked in zip(self.values, self.mask) if not masked]


class FakeNumpy(types.ModuleType):
    def __init__(self) -> None:
        super().__init__("numpy")
        self.ma = types.SimpleNamespace(array=MaskedArray, masked=MASKED)

    def min(self, values: Any) -> Any:  # noqa: A003 - match numpy API
        return self._reduce(values, minimum=True)

    def max(self, values: Any) -> Any:  # noqa: A003 - match numpy API
        return self._reduce(values, minimum=False)

    @staticmethod
    def _reduce(values: Any, *, minimum: bool) -> Any:
        if isinstance(values, MaskedArray):
            candidates = values.unmasked_values()
            if not candidates:
                return MASKED
        else:
            candidates = [float(value) for value in values]
            if not candidates:
                raise ValueError("zero-size array to reduction operation minimum/maximum which has no identity")

        for value in candidates:
            if math.isnan(value):
                return float("nan")
        return min(candidates) if minimum else max(candidates)


class EncodedValue:
    def __init__(self, value: Any) -> None:
        self.value = value

    def to_json(self) -> dict[str, Any]:
        if isinstance(self.value, MaskedScalar):
            return {"kind": "masked"}
        if isinstance(self.value, float) and math.isnan(self.value):
            return {"kind": "nan"}
        return {"kind": "number", "value": self.value}


def install_fake_numpy() -> None:
    sys.modules["numpy"] = FakeNumpy()


def load_upstream_plotdata(reference_root: Path) -> type[Any]:
    source_path = reference_root / "pyqtgraph" / "PlotData.py"
    if not source_path.exists():
        raise SystemExit(f"Missing pinned PyQtGraph source: {source_path}")

    install_fake_numpy()
    spec = importlib.util.spec_from_file_location("pyqtgraph.PlotData", source_path)
    if spec is None or spec.loader is None:
        raise SystemExit(f"Unable to load {source_path}")
    module = importlib.util.module_from_spec(spec)
    module.__package__ = "pyqtgraph"
    sys.modules["pyqtgraph.PlotData"] = module
    spec.loader.exec_module(module)
    return module.PlotData


def exception_name(callable_: Callable[[], Any]) -> str:
    try:
        callable_()
    except Exception as exc:  # noqa: BLE001 - fixture records upstream exception names.
        return type(exc).__name__
    raise AssertionError("expected exception")


def extrema_case(PlotData: type[Any], field: str, values: Any) -> dict[str, Any]:
    data = PlotData()
    data[field] = values
    return {
        "input_kind": type(values).__name__,
        "min": EncodedValue(data.min(field)).to_json(),
        "max": EncodedValue(data.max(field)).to_json(),
    }


def build_fixture_from_source(reference_root: Path) -> dict[str, Any]:
    PlotData = load_upstream_plotdata(reference_root)
    np = sys.modules["numpy"]

    data = PlotData()
    data.addFields(x=None, y=None)
    data["x"] = [3.0, -2.0, 5.0]
    first_min = data.min("x")
    first_max = data.max("x")
    data["x"] = [10.0, 12.0]

    return fixture_payload(
        {
            "added_field_default_is_none": data["y"] is None,
            "finite_list": extrema_case(PlotData, "finite_list", [3.0, -2.0, 5.0]),
            "tuple_container": extrema_case(PlotData, "tuple_container", (8.0, -4.0, 6.0)),
            "range_container": extrema_case(PlotData, "range_container", range(1, 5)),
            "nan_propagates": extrema_case(PlotData, "nan_propagates", [1.0, float("nan"), -3.0]),
            "empty_exception": exception_name(lambda: extrema_case(PlotData, "empty", [])),
            "masked_partial": extrema_case(
                PlotData,
                "masked_partial",
                np.ma.array([7.0, -5.0, 2.0, float("nan")], mask=[False, True, False, True]),
            ),
            "all_masked": extrema_case(
                PlotData,
                "all_masked",
                np.ma.array([1.0, 2.0], mask=[True, True]),
            ),
            "stale_cache_after_assignment": {
                "first_min": EncodedValue(first_min).to_json(),
                "first_max": EncodedValue(first_max).to_json(),
                "after_replace_min": EncodedValue(data.min("x")).to_json(),
                "after_replace_max": EncodedValue(data.max("x")).to_json(),
            },
        }
    )


def fixture_payload(cases: dict[str, Any]) -> dict[str, Any]:
    return {
        "schema_version": 1,
        "issue": "P2.03",
        "upstream": {
            "project": "pyqtgraph",
            "ref": PINNED_REF,
            "commit": PINNED_COMMIT,
            "files": UPSTREAM_FILES,
            "probe_note": "Executes pyqtgraph/PlotData.py with a minimal NumPy reduction stand-in for list/range/NaN/masked cases.",
        },
        "tolerances": {"absolute": 0.0, "nan": "both_nan"},
        "cases": cases,
        "cpp_deviations": [
            "C++ stores one-dimensional double arrays instead of arbitrary Python objects; addFields creates empty arrays instead of None.",
            "C++ all-masked extrema throw std::invalid_argument because the public min/max API returns double and cannot represent NumPy's masked scalar.",
            "C++ set()/setMasked() intentionally preserve PyQtGraph PlotData.py stale min/max caches after replacement.",
        ],
    }


def fallback_fixture() -> dict[str, Any]:
    return fixture_payload(
        {
            "added_field_default_is_none": True,
            "finite_list": {
                "input_kind": "list",
                "min": {"kind": "number", "value": -2.0},
                "max": {"kind": "number", "value": 5.0},
            },
            "tuple_container": {
                "input_kind": "tuple",
                "min": {"kind": "number", "value": -4.0},
                "max": {"kind": "number", "value": 8.0},
            },
            "range_container": {
                "input_kind": "range",
                "min": {"kind": "number", "value": 1.0},
                "max": {"kind": "number", "value": 4.0},
            },
            "nan_propagates": {
                "input_kind": "list",
                "min": {"kind": "nan"},
                "max": {"kind": "nan"},
            },
            "empty_exception": "ValueError",
            "masked_partial": {
                "input_kind": "MaskedArray",
                "min": {"kind": "number", "value": 2.0},
                "max": {"kind": "number", "value": 7.0},
            },
            "all_masked": {
                "input_kind": "MaskedArray",
                "min": {"kind": "masked"},
                "max": {"kind": "masked"},
            },
            "stale_cache_after_assignment": {
                "first_min": {"kind": "number", "value": -2.0},
                "first_max": {"kind": "number", "value": 5.0},
                "after_replace_min": {"kind": "number", "value": -2.0},
                "after_replace_max": {"kind": "number", "value": 5.0},
            },
        }
    )


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
        print(f"P2.03 oracle fixture OK: {FIXTURE.relative_to(ROOT)}{source_note}")
        return 0

    FIXTURE.parent.mkdir(parents=True, exist_ok=True)
    FIXTURE.write_text(expected, encoding="utf-8")
    print(f"Wrote {FIXTURE.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
