#!/usr/bin/env python3
"""Generate/check the P414 ImageView menu, normalization, and export oracle."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import sys
import tempfile
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
SOURCE_LOCK = ROOT / "reference" / "source.lock"
IMAGEVIEW = ROOT / "reference" / "pyqtgraph" / "pyqtgraph" / "imageview" / "ImageView.py"
FIXTURE = ROOT / "oracle" / "fixtures" / "P414" / "imageview_menu_norm_export_oracle.json"
PINNED_REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"
FRAMES = 4
HEIGHT = 8
WIDTH = 8


def source_paths_available() -> bool:
    return all(path.exists() for path in (SOURCE_LOCK, IMAGEVIEW))


def pyqtgraph_runtime_available() -> bool:
    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    pinned_checkout = ROOT / "reference" / "pyqtgraph"
    if str(pinned_checkout) not in sys.path:
        sys.path.insert(0, str(pinned_checkout))
    try:
        import pyqtgraph  # noqa: F401
    except ImportError:
        return False
    return True


def require_pinned_sources() -> None:
    missing = [path for path in (SOURCE_LOCK, IMAGEVIEW) if not path.exists()]
    if missing:
        names = ", ".join(str(path.relative_to(ROOT)) for path in missing)
        raise SystemExit(f"Pinned PyQtGraph checkout is unavailable; missing {names}")

    lock_text = SOURCE_LOCK.read_text(encoding="utf-8")
    if PINNED_COMMIT not in lock_text or PINNED_REF not in lock_text:
        raise SystemExit("reference/source.lock does not match the P414 pinned PyQtGraph ref/commit")


def build_fixture_data() -> tuple[Any, Any]:
    import numpy as np

    data = np.zeros((FRAMES, HEIGHT, WIDTH), dtype=np.float32)
    for frame in range(FRAMES):
        data[frame, :, :] = 100.0 + 10.0 * frame
    xvals = np.array([0.0, 1.5, 3.0, 5.0], dtype=np.float64)
    return data, xvals


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    digest.update(path.read_bytes())
    return digest.hexdigest()


def curve_payload(imv: Any) -> list[dict[str, Any]]:
    curves: list[dict[str, Any]] = []
    for index, curve in enumerate(imv.roiCurves):
        xvals, yvals = curve.getData()
        curves.append(
            {
                "channel": index,
                "x": [float(value) for value in xvals],
                "y": [float(value) for value in yvals],
            }
        )
    return curves


def render_oracle() -> dict[str, Any]:
    import numpy as np

    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    pinned_checkout = ROOT / "reference" / "pyqtgraph"
    if str(pinned_checkout) not in sys.path:
        sys.path.insert(0, str(pinned_checkout))
    import pyqtgraph as pg
    from pyqtgraph.Qt import QtWidgets

    data, _xvals = build_fixture_data()
    app = pg.mkQApp("P414 ImageView menu norm export oracle")
    win = QtWidgets.QMainWindow()
    win.resize(800, 800)
    imv = pg.ImageView()
    win.setCentralWidget(imv)
    win.show()
    imv.setImage(data)
    for _ in range(5):
        app.processEvents()

    if imv.menu is None:
        imv.buildMenu()
    menu_actions = [action.text() for action in imv.menu.actions()]

    imv.normAction.setChecked(True)
    for _ in range(5):
        app.processEvents()
    norm_group_visible = imv.ui.normGroup.isVisible()

    imv.ui.normDivideRadio.setChecked(True)
    imv.ui.normFrameCheck.setChecked(True)
    imv.normRadioChanged()
    for _ in range(5):
        app.processEvents()

    processed = imv.getProcessedImage()
    sample_pixel = float(processed[0, 1, 1])

    imv.ui.roiBtn.setChecked(True)
    imv.roiClicked()
    imv.roiChanged()
    for _ in range(5):
        app.processEvents()
    startup_curves = curve_payload(imv)

    with tempfile.TemporaryDirectory(prefix="p414-export-") as tmpdir:
        export_base = Path(tmpdir) / "stack.png"
        imv.export(str(export_base))
        export_files = sorted(Path(tmpdir).glob("stack*.png"))
        export_hashes = [sha256_file(path) for path in export_files]

    return {
        "pinned_ref": PINNED_REF,
        "pinned_commit": PINNED_COMMIT,
        "frames": FRAMES,
        "height": HEIGHT,
        "width": WIDTH,
        "xvals": [float(index) for index in range(FRAMES)],
        "menu_actions": menu_actions,
        "norm_group_visible": norm_group_visible,
        "normalized_sample_pixel": sample_pixel,
        "startup_curves": startup_curves,
        "export_file_count": len(export_hashes),
        "export_hashes": export_hashes,
    }


def nearly_equal(lhs: float, rhs: float, tolerance: float = 1.0e-5) -> bool:
    return math.isclose(lhs, rhs, rel_tol=0.0, abs_tol=tolerance)


def check_curves(actual: list[dict[str, Any]], expected: list[dict[str, Any]]) -> None:
    if len(actual) != len(expected):
        raise SystemExit(f"curve count mismatch: expected {len(expected)}, got {len(actual)}")
    for actual_curve, expected_curve in zip(actual, expected, strict=True):
        for key in ("x", "y"):
            actual_values = actual_curve[key]
            expected_values = expected_curve[key]
            if len(actual_values) != len(expected_values):
                raise SystemExit(
                    f"curve {actual_curve['channel']} {key} length mismatch: "
                    f"expected {len(expected_values)}, got {len(actual_values)}"
                )
            for index, (lhs, rhs) in enumerate(zip(actual_values, expected_values, strict=True)):
                if not nearly_equal(lhs, rhs):
                    raise SystemExit(
                        f"curve {actual_curve['channel']} {key}[{index}] mismatch: expected {rhs}, got {lhs}"
                    )


def check_fixture(path: Path, *, verify_against_source: bool) -> None:
    if not path.is_file():
        raise SystemExit(f"missing fixture: {path}")
    payload = json.loads(path.read_text(encoding="utf-8"))
    for key in (
        "pinned_ref",
        "pinned_commit",
        "menu_actions",
        "norm_group_visible",
        "normalized_sample_pixel",
        "startup_curves",
        "export_file_count",
        "export_hashes",
    ):
        if key not in payload:
            raise SystemExit(f"fixture missing key: {key}")
    if payload["pinned_ref"] != PINNED_REF or payload["pinned_commit"] != PINNED_COMMIT:
        raise SystemExit("fixture pinned ref/commit mismatch")
    if not verify_against_source:
        return

    rendered = render_oracle()
    if rendered["menu_actions"] != payload["menu_actions"]:
        raise SystemExit(
            f"menu actions mismatch: expected {payload['menu_actions']}, got {rendered['menu_actions']}"
        )
    if rendered["norm_group_visible"] != payload["norm_group_visible"]:
        raise SystemExit("norm group visibility mismatch")
    if not nearly_equal(rendered["normalized_sample_pixel"], payload["normalized_sample_pixel"]):
        raise SystemExit(
            "normalized sample pixel mismatch: "
            f"expected {payload['normalized_sample_pixel']}, got {rendered['normalized_sample_pixel']}"
        )
    check_curves(rendered["startup_curves"], payload["startup_curves"])
    if rendered["export_file_count"] != payload["export_file_count"]:
        raise SystemExit("export file count mismatch")
    if rendered["export_hashes"] != payload["export_hashes"]:
        raise SystemExit("export hashes mismatch")


def write_fixture(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = render_oracle()
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {path}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="write the pinned fixture")
    parser.add_argument("--check", action="store_true", help="validate the pinned fixture")
    args = parser.parse_args()
    if not any((args.write, args.check)):
        parser.error("one of --write or --check is required")

    require_pinned_sources()
    if args.write:
        if not pyqtgraph_runtime_available():
            raise SystemExit("PyQtGraph runtime unavailable for --write")
        write_fixture(FIXTURE)
    if args.check:
        verify = pyqtgraph_runtime_available()
        check_fixture(FIXTURE, verify_against_source=verify)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
