#!/usr/bin/env python3
"""Generate/check the P413 ImageView ROI plot oracle from pinned PyQtGraph."""

from __future__ import annotations

import argparse
import json
import math
import os
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
SOURCE_LOCK = ROOT / "reference" / "source.lock"
EXAMPLE = ROOT / "reference" / "pyqtgraph" / "pyqtgraph" / "examples" / "ImageView.py"
IMAGEVIEW = ROOT / "reference" / "pyqtgraph" / "pyqtgraph" / "imageview" / "ImageView.py"
FIXTURE = ROOT / "oracle" / "fixtures" / "P413" / "imageview_roi_plot_oracle.json"
REFERENCE_CROP = ROOT / "oracle" / "fixtures" / "P413" / "screenshots" / "image_area.reference.png"
PINNED_REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"
FRAMES = 4
HEIGHT = 4
WIDTH = 4
CHANNELS = 3


def source_paths_available() -> bool:
    return all(path.exists() for path in (SOURCE_LOCK, EXAMPLE, IMAGEVIEW))


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
    missing = [path for path in (SOURCE_LOCK, EXAMPLE, IMAGEVIEW) if not path.exists()]
    if missing:
        names = ", ".join(str(path.relative_to(ROOT)) for path in missing)
        raise SystemExit(f"Pinned PyQtGraph checkout is unavailable; missing {names}")

    lock_text = SOURCE_LOCK.read_text(encoding="utf-8")
    if PINNED_COMMIT not in lock_text or PINNED_REF not in lock_text:
        raise SystemExit("reference/source.lock does not match the P413 pinned PyQtGraph ref/commit")


def build_fixture_data() -> tuple[Any, Any]:
    import numpy as np

    data = np.zeros((FRAMES, HEIGHT, WIDTH, CHANNELS), dtype=np.float32)
    for frame in range(FRAMES):
        for channel in range(CHANNELS):
            data[frame, :, :, channel] = 100.0 + 10.0 * frame + 5.0 * channel
    xvals = np.array([0.0, 1.5, 3.0, 5.0], dtype=np.float64)
    return data, xvals


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
    from pyqtgraph.Qt import QtCore, QtWidgets

    data, xvals = build_fixture_data()
    app = pg.mkQApp("P413 ImageView ROI plot oracle")
    win = QtWidgets.QMainWindow()
    win.resize(800, 800)
    imv = pg.ImageView(levelMode="rgba")
    win.setCentralWidget(imv)
    win.show()
    imv.setImage(data, xvals=xvals)
    imv.ui.roiBtn.setChecked(True)
    imv.roiClicked()
    for _ in range(5):
        app.processEvents()

    startup_curves = curve_payload(imv)

    imv.roi.setSize([2.0, 2.0])
    imv.roiChanged()
    for _ in range(5):
        app.processEvents()
    resized_curves = curve_payload(imv)

    graphics_view = imv.ui.graphicsView
    top_left = graphics_view.mapTo(win.centralWidget(), QtCore.QPoint(0, 0))
    crop = {
        "x": int(top_left.x()),
        "y": int(top_left.y()),
        "width": int(graphics_view.width()),
        "height": int(graphics_view.height()),
    }

    return {
        "pinned_ref": PINNED_REF,
        "pinned_commit": PINNED_COMMIT,
        "frames": FRAMES,
        "height": HEIGHT,
        "width": WIDTH,
        "channels": CHANNELS,
        "xvals": [float(value) for value in xvals],
        "window": {"width": 800, "height": 800},
        "image_crop": crop,
        "startup": {
            "roi_checked": True,
            "roi_visible": imv.roi.isVisible(),
            "roi_plot_visible": imv.ui.roiPlot.isVisible(),
            "curves": startup_curves,
        },
        "resized_roi": {
            "size": [2.0, 2.0],
            "curves": resized_curves,
        },
    }


def nearly_equal(lhs: float, rhs: float, tolerance: float = 1.0e-6) -> bool:
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
    for key in ("pinned_ref", "pinned_commit", "frames", "height", "width", "channels", "xvals", "startup"):
        if key not in payload:
            raise SystemExit(f"fixture missing key: {key}")
    if payload["pinned_ref"] != PINNED_REF or payload["pinned_commit"] != PINNED_COMMIT:
        raise SystemExit("fixture pinned ref/commit mismatch")
    if not verify_against_source:
        return

    rendered = render_oracle()
    check_curves(rendered["startup"]["curves"], payload["startup"]["curves"])
    check_curves(rendered["resized_roi"]["curves"], payload["resized_roi"]["curves"])


def write_fixture(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = render_oracle()
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {path}")


def write_reference_crop(path: Path) -> None:
    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    pinned_checkout = ROOT / "reference" / "pyqtgraph"
    if str(pinned_checkout) not in sys.path:
        sys.path.insert(0, str(pinned_checkout))
    import pyqtgraph as pg
    from pyqtgraph.Qt import QtCore, QtGui, QtWidgets

    data, xvals = build_fixture_data()
    app = pg.mkQApp("P413 ImageView ROI visual oracle")
    win = QtWidgets.QMainWindow()
    win.resize(800, 800)
    imv = pg.ImageView(levelMode="rgba")
    win.setCentralWidget(imv)
    win.show()
    imv.setImage(data, xvals=xvals)
    imv.ui.roiBtn.setChecked(True)
    imv.roiClicked()
    for _ in range(5):
        app.processEvents()

    pixmap = win.grab()
    path.parent.mkdir(parents=True, exist_ok=True)
    if not pixmap.save(str(path)):
        raise SystemExit(f"failed to write reference screenshot: {path}")
    print(f"wrote {path}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="write the pinned fixture")
    parser.add_argument("--write-reference", action="store_true", help="write the reference screenshot")
    parser.add_argument("--check", action="store_true", help="validate the pinned fixture")
    args = parser.parse_args()
    if not any((args.write, args.write_reference, args.check)):
        parser.error("one of --write, --write-reference, or --check is required")

    if args.write or args.write_reference or args.check:
        require_pinned_sources()

    if args.write:
        if not pyqtgraph_runtime_available():
            raise SystemExit("PyQtGraph runtime unavailable for --write")
        write_fixture(FIXTURE)
    if args.write_reference:
        if not pyqtgraph_runtime_available():
            raise SystemExit("PyQtGraph runtime unavailable for --write-reference")
        write_reference_crop(REFERENCE_CROP)
    if args.check:
        verify = pyqtgraph_runtime_available()
        check_fixture(FIXTURE, verify_against_source=verify)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
