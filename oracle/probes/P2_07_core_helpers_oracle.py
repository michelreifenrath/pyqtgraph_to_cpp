#!/usr/bin/env python3
"""Generate/check the P2.07 config/debug/exception/units oracle fixture.

The checked fixture records the focused PyQtGraph 0.14.0 helper behavior used
by the native C++ proof.  When a pinned checkout is supplied, this probe also
verifies that the expected upstream files and behavioral markers are present.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_REF_ROOT = ROOT / "reference" / "pyqtgraph"
FIXTURE = ROOT / "oracle" / "fixtures" / "P2_07" / "core_helpers_oracle.json"
PINNED_REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"
UPSTREAM_FILES = [
    "pyqtgraph/__init__.py",
    "pyqtgraph/configfile.py",
    "pyqtgraph/debug.py",
    "pyqtgraph/exceptionHandling.py",
    "pyqtgraph/units.py",
    "tests/test_configparser.py",
]


def fixture_payload() -> dict[str, Any]:
    return {
        "schema_version": 1,
        "issue": "P2.07",
        "upstream": {
            "project": "pyqtgraph",
            "ref": PINNED_REF,
            "commit": PINNED_COMMIT,
            "files": UPSTREAM_FILES,
            "probe_note": "Config option defaults and validation come from __init__.py; config text parsing cases come from configfile.py and tests/test_configparser.py; units come from units.py; debug/exception callback contracts come from debug.py and exceptionHandling.py.",
        },
        "tolerances": {
            "unit_scale_absolute": 1.0e-15,
            "config_roundtrip": "exact native ConfigValue equality",
            "exception_callback_order": "registration order; callback failures isolated and counted",
        },
        "cases": {
            "CONFIG_OPTIONS": {
                "defaults": {
                    "useOpenGL": False,
                    "leftButtonPan": True,
                    "foreground": "d",
                    "background": "k",
                    "antialias": False,
                    "editorCommand": None,
                    "exitCleanup": True,
                    "enableExperimental": False,
                    "crashWarning": False,
                    "mouseRateLimit": 100,
                    "imageAxisOrder": "col-major",
                    "useCupy": False,
                    "useNumba": False,
                    "segmentedLineMode": "auto",
                },
                "imageAxisOrder_accepts": ["row-major", "col-major"],
                "segmentedLineMode_accepts": ["auto", "on", "off"],
                "unknown_option": "rejected",
                "invalid_enums": "rejected with std::invalid_argument C++ equivalent of PyQtGraph ValueError",
            },
            "units": {
                "SI_PREFIXES_no_prefix": "literal space sentinel exposed publicly; allUnits translates it to an empty unit-map prefix",
                "scales": {
                    "m": 1.0,
                    "mm": 0.001,
                    "µm": 0.000001,
                    "μm": 0.000001,
                    "um": 0.000001,
                    "kHz": 1000.0,
                    "MHz": 1000000.0,
                    "Ohm": 1.0,
                    "Ω": 1.0,
                    "mV": 0.001,
                    "daV": 10.0,
                    "hPa": 100.0,
                    "dB": 0.1,
                    "cA": 0.01,
                },
                "upstream_stubs": ["evalUnits", "formatUnits", "simplify"],
            },
            "configfile": {
                "roundtrip": "nested native scalar/list/map values",
                "float_repr": "floating values that print as integral numbers retain .0 syntax for exact round-trip type parity",
                "duplicate_key_error": "message contains Duplicate key",
                "line_numbers": "comments and blanks count; duplicate on physical line 4 reports line 4",
                "comment_indentation": "comment-only lines do not affect nested indentation",
                "inline_comments": "trailing # comments outside quoted strings are ignored before literal parsing",
                "key_validation": "blank keys, leading spaces, and ':' in keys rejected by genString",
            },
            "exceptionHandling": {
                "callbacks": "registerCallback/unregisterCallback preserve order",
                "deprecated_callbacks": "register/unregister native equivalent preserved",
                "traceback_clearing": "setTracebackClearing toggles stored flag; Python sys.last_traceback clearing is not installed in C++",
            },
            "debug": {
                "formatException": "includes native exception type and message",
                "warnOnException": "suppresses a callable exception and reports false",
                "Profiler": "RAII elapsed-time helper; no Python cProfile/GC integration",
            },
        },
        "cpp_deviations": [
            "C++ config values are std::variant-backed native values; editorCommand None is std::monostate.",
            "configfile parsing intentionally supports deterministic native literals (None/bool/int/double/quoted string/list/nested map) and rejects arbitrary Python eval, NumPy array, Point, ColorMap, QtCore, and tuple-key reconstruction.",
            "debug.py Python GC/referrer/trace/sys.settrace/faulthandler helpers are documented as unsupported native no-ops rather than Python wrappers.",
            "exceptionHandling.py sys.excepthook/threading.excepthook installation is represented by explicit notifyUnhandledException/ExceptionHandler::handle calls under exceptionHandling.hpp in this capped C++ proof; no global std::terminate hook is installed.",
            "units.py evalUnits/formatUnits/simplify are upstream stubs in PyQtGraph 0.14.0; C++ returns std::nullopt instead of inventing unit algebra.",
        ],
    }


def validate_reference_root(reference_root: Path) -> None:
    missing = [path for path in (reference_root / file for file in UPSTREAM_FILES) if not path.exists()]
    if missing:
        raise SystemExit("Missing pinned PyQtGraph source: " + ", ".join(str(path) for path in missing))

    markers = {
        "pyqtgraph/__init__.py": ["CONFIG_OPTIONS = {", "def setConfigOption", "segmentedLineMode"],
        "pyqtgraph/configfile.py": ["class ParseError", "def genString", "def parseString", "Duplicate key"],
        "pyqtgraph/units.py": ["SI_PREFIXES", "addUnit(\"u\", 1e-6)", "def evalUnits"],
        "pyqtgraph/debug.py": ["def warnOnException", "def formatException", "class Profiler"],
        "pyqtgraph/exceptionHandling.py": ["callbacks = []", "def registerCallback", "class ExceptionHandler"],
        "tests/test_configparser.py": ["test_duplicate_keys_error", "test_comment_indentation_is_ignored"],
    }
    for rel, required in markers.items():
        text = (reference_root / rel).read_text(encoding="utf-8")
        missing_markers = [marker for marker in required if marker not in text]
        if missing_markers:
            raise SystemExit(f"Pinned {rel} does not contain expected markers: {', '.join(missing_markers)}")


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
        print(f"P2.07 oracle fixture OK: {FIXTURE.relative_to(ROOT)}{source_note}")
        return 0

    FIXTURE.parent.mkdir(parents=True, exist_ok=True)
    FIXTURE.write_text(expected, encoding="utf-8")
    print(f"Wrote {FIXTURE.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
