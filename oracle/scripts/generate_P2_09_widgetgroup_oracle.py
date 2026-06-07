#!/usr/bin/env python3
"""Generate/check the P2.09 WidgetGroup oracle fixture.

The fixture records deterministic WidgetGroup behavior from the pinned
PyQtGraph 0.14.0 source without importing PyQtGraph or a Qt binding.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE_LOCK = ROOT / "reference" / "source.lock"
FIXTURE = ROOT / "oracle" / "fixtures" / "P2_09" / "widgetgroup_oracle.json"
PINNED_REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"


def widgetgroup_source_candidates() -> list[Path]:
    opensrc_home = Path(os.environ.get("OPENSRC_HOME", Path.home() / ".cache" / "pgcpp-opensrc"))
    return [
        ROOT / "reference" / "pyqtgraph" / "pyqtgraph" / "WidgetGroup.py",
        opensrc_home
        / "repos"
        / "github.com"
        / "pyqtgraph"
        / "pyqtgraph"
        / PINNED_REF
        / "pyqtgraph"
        / "WidgetGroup.py",
    ]


def available_widgetgroup_source() -> Path | None:
    for path in widgetgroup_source_candidates():
        if path.exists():
            return path
    return None


def source_paths_available() -> bool:
    return SOURCE_LOCK.exists() and available_widgetgroup_source() is not None


def require_pinned_sources() -> None:
    source_path = available_widgetgroup_source()
    missing: list[str] = []
    if not SOURCE_LOCK.exists():
        missing.append(str(SOURCE_LOCK.relative_to(ROOT)))
    if source_path is None:
        missing.extend(str(path) for path in widgetgroup_source_candidates())
    if missing:
        raise SystemExit(f"Pinned PyQtGraph checkout is unavailable; missing {', '.join(missing)}")

    lock_text = SOURCE_LOCK.read_text(encoding="utf-8")
    if PINNED_COMMIT not in lock_text or PINNED_REF not in lock_text:
        raise SystemExit(
            "reference/source.lock does not match the P2.09 pinned PyQtGraph ref/commit"
        )

    source_text = source_path.read_text(encoding="utf-8")
    required_snippets = [
        "w.saveState().toPercentEncoding().data().decode()",
        "w.restoreState(QtCore.QByteArray.fromPercentEncoding(s.encode()))",
        "w.setSizes([50] * w.count())",
        "return str(w.itemText(ind))",
        "if type(v) is int:",
        "QtWidgets.QSpinBox",
        "QtWidgets.QDoubleSpinBox",
        "QtWidgets.QSplitter",
        "QtWidgets.QCheckBox",
        "QtWidgets.QComboBox",
        "QtWidgets.QGroupBox",
        "QtWidgets.QLineEdit",
        "QtWidgets.QRadioButton",
        "QtWidgets.QSlider",
        "self.cache.copy()",
        "if n not in s:",
        "val /= self.scales[w]",
        "v *= self.scales[w]",
        "self.sigChanged.emit(self.widgetList[w], v2)",
    ]
    for snippet in required_snippets:
        if snippet not in source_text:
            raise SystemExit(
                f"Pinned source did not contain expected WidgetGroup snippet: {snippet!r}"
            )


def build_fixture() -> dict[str, object]:
    return {
        "issue": "P2.09",
        "upstream": {
            "project": "pyqtgraph",
            "ref": PINNED_REF,
            "commit": PINNED_COMMIT,
            "files": ["pyqtgraph/WidgetGroup.py"],
            "source_lines": {
                "splitter_state_restore": "17-32",
                "combo_state_restore": "34-53",
                "supported_classes": "75-120",
                "constructor_add_autoadd": "128-180",
                "state_setstate_scale_signal": "231-281",
            },
        },
        "behavior_tolerance": "exact QVariant-compatible state equality; floating scale values compared within 1e-12 in C++ tests",
        "supported_builtin_widgets": [
            "QSpinBox",
            "QDoubleSpinBox",
            "QSplitter",
            "QCheckBox",
            "QComboBox",
            "QGroupBox",
            "QLineEdit",
            "QRadioButton",
            "QSlider",
        ],
        "cases": [
            {
                "name": "named_builtin_roundtrip",
                "behavior": "state is keyed by explicit name or objectName and setState restores only matching keys",
            },
            {
                "name": "combo_data_then_text",
                "behavior": "save returns current item data when valid, otherwise current item text; integer restore searches item data before text",
            },
            {
                "name": "scale_save_restore",
                "behavior": "read divides by scale and restore multiplies by scale",
            },
            {
                "name": "unknown_state_keys_ignored",
                "behavior": "setState ignores state entries with no matching widget name",
            },
            {
                "name": "sig_changed_on_cache_delta",
                "behavior": "change signal refreshes the cache and emits sigChanged only when the value differs",
            },
            {
                "name": "splitter_uncached_state",
                "behavior": "splitter has no change signal, state refreshes from saveState, supports percent-encoded string and list restore, and all-zero sizes are replaced by [50] * count",
            },
            {
                "name": "autoadd_recursion",
                "behavior": "autoAdd recurses through generic QObject children and through accepted QSplitter/QGroupBox children",
            },
        ],
        "declared_cpp_deviations": [
            "Python custom widgetGroupInterface hook is not ported in this shard; the C++ API supports the built-in Qt widget set above."
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--check",
        action="store_true",
        help="verify the fixture is current instead of writing it",
    )
    parser.add_argument(
        "--require-source",
        action="store_true",
        help="fail if the optional pinned PyQtGraph checkout is absent",
    )
    args = parser.parse_args()

    expected = json.dumps(build_fixture(), indent=2, sort_keys=True) + "\n"

    if args.require_source or not args.check or source_paths_available():
        require_pinned_sources()

    if args.check:
        if not FIXTURE.exists():
            raise SystemExit(f"Missing oracle fixture: {FIXTURE.relative_to(ROOT)}")
        actual = FIXTURE.read_text(encoding="utf-8")
        if actual != expected:
            raise SystemExit(
                f"Oracle fixture is stale: regenerate {FIXTURE.relative_to(ROOT)}"
            )
        print(f"P2.09 oracle fixture OK: {FIXTURE.relative_to(ROOT)}")
        return 0

    FIXTURE.parent.mkdir(parents=True, exist_ok=True)
    FIXTURE.write_text(expected, encoding="utf-8")
    print(f"Wrote {FIXTURE.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
