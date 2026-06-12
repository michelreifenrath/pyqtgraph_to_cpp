#!/usr/bin/env python3
"""Generate/check the P409 ImageView view-transform oracle from pinned PyQtGraph."""

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
VIEWBOX = ROOT / "reference" / "pyqtgraph" / "pyqtgraph" / "graphicsItems" / "ViewBox" / "ViewBox.py"
FIXTURE = ROOT / "oracle" / "fixtures" / "P409" / "imageview_view_oracle.json"
REFERENCE_CROP = ROOT / "oracle" / "fixtures" / "P409" / "screenshots" / "image_area.reference.png"
PINNED_REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"
FRAMES = 100
HEIGHT = 200
WIDTH = 200


def source_paths_available() -> bool:
    return all(path.exists() for path in (SOURCE_LOCK, EXAMPLE, IMAGEVIEW, VIEWBOX))


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
    missing = [path for path in (SOURCE_LOCK, EXAMPLE, IMAGEVIEW, VIEWBOX) if not path.exists()]
    if missing:
        names = ", ".join(str(path.relative_to(ROOT)) for path in missing)
        raise SystemExit(f"Pinned PyQtGraph checkout is unavailable; missing {names}")

    lock_text = SOURCE_LOCK.read_text(encoding="utf-8")
    if PINNED_COMMIT not in lock_text or PINNED_REF not in lock_text:
        raise SystemExit("reference/source.lock does not match the P409 pinned PyQtGraph ref/commit")


def build_example_data() -> tuple[Any, Any]:
    import numpy as np

    data = np.zeros((FRAMES, HEIGHT, WIDTH, 3), dtype=np.float32)
    for frame in range(FRAMES):
        data[frame, :, :, 0] = 90.0 + 60.0 * frame / (FRAMES - 1)
        data[frame, :, :, 1] = 90.0 + 90.0 * frame / (FRAMES - 1)
        data[frame, :, :, 2] = 180.0 - 90.0 * frame / (FRAMES - 1)
    xvals = np.linspace(1.0, 3.0, FRAMES)
    return data, xvals


def render_oracle() -> dict[str, Any]:
    import numpy as np

    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    pinned_checkout = ROOT / "reference" / "pyqtgraph"
    if str(pinned_checkout) not in sys.path:
        sys.path.insert(0, str(pinned_checkout))
    import pyqtgraph as pg
    from pyqtgraph.Qt import QtCore, QtWidgets

    data, xvals = build_example_data()
    pg.setConfigOptions(imageAxisOrder="row-major")
    app = pg.mkQApp("P409 ImageView view oracle")
    win = QtWidgets.QMainWindow()
    win.resize(800, 800)
    imv = pg.ImageView()
    win.setCentralWidget(imv)
    win.setWindowTitle("pyqtgraph example: ImageView")
    win.show()
    imv.setImage(data, xvals=xvals)
    for _ in range(5):
        app.processEvents()

    vb = imv.view
    view_range = vb.viewRange()
    graphics_view = imv.ui.graphicsView
    top_left = graphics_view.mapTo(win.centralWidget(), QtCore.QPoint(0, 0))
    crop = {
        "x": int(top_left.x()),
        "y": int(top_left.y()),
        "width": int(graphics_view.width()),
        "height": int(graphics_view.height()),
    }
    return {
        "issue": "P409",
        "source": {
            "id": PINNED_REF,
            "pinned_commit": PINNED_COMMIT,
            "files": [
                "pyqtgraph/examples/ImageView.py",
                "pyqtgraph/imageview/ImageView.py",
                "pyqtgraph/graphicsItems/ViewBox/ViewBox.py",
            ],
        },
        "window": {"width": 800, "height": 800},
        "shape": [FRAMES, HEIGHT, WIDTH, 3],
        "y_inverted": bool(vb.yInverted()),
        "view_range": {
            "x": [float(view_range[0][0]), float(view_range[0][1])],
            "y": [float(view_range[1][0]), float(view_range[1][1])],
        },
        "view_range_tolerance": 25.0,
        "aspect_ratio_tolerance": 0.02,
        "image_crop": crop,
        "visual_tolerance": {
            "max_mean_delta": 12.0,
            "max_pixel_delta": 255.0,
            "max_changed_percent": 8.0,
        },
    }


def write_reference_crop() -> None:
    import numpy as np

    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    pinned_checkout = ROOT / "reference" / "pyqtgraph"
    if str(pinned_checkout) not in sys.path:
        sys.path.insert(0, str(pinned_checkout))
    import pyqtgraph as pg
    from pyqtgraph.Qt import QtCore, QtWidgets

    data, xvals = build_example_data()
    pg.setConfigOptions(imageAxisOrder="row-major")
    app = pg.mkQApp("P409 ImageView visual reference")
    win = QtWidgets.QMainWindow()
    win.resize(800, 800)
    imv = pg.ImageView()
    win.setCentralWidget(imv)
    win.show()
    imv.setImage(data, xvals=xvals)
    for _ in range(5):
        app.processEvents()

    graphics_view = imv.ui.graphicsView
    top_left = graphics_view.mapTo(win.centralWidget(), QtCore.QPoint(0, 0))
    pixmap = win.grab()
    image = pixmap.toImage()
    cropped = image.copy(top_left.x(), top_left.y(), graphics_view.width(), graphics_view.height())
    REFERENCE_CROP.parent.mkdir(parents=True, exist_ok=True)
    if not cropped.save(str(REFERENCE_CROP), "PNG"):
        raise SystemExit(f"failed to write ImageView reference crop: {REFERENCE_CROP}")


def nearly_equal(lhs: float, rhs: float, tolerance: float) -> bool:
    return math.isclose(lhs, rhs, abs_tol=tolerance)


def check_fixture(path: Path, verify_against_source: bool = True) -> None:
    data = json.loads(path.read_text(encoding="utf-8"))
    source = data.get("source", {})
    assert source.get("id") == PINNED_REF
    assert source.get("pinned_commit") == PINNED_COMMIT
    assert data["window"] == {"width": 800, "height": 800}
    assert data["shape"] == [FRAMES, HEIGHT, WIDTH, 3]
    assert data["y_inverted"] is True

    view_range = data["view_range"]
    tolerance = float(data["view_range_tolerance"])
    for axis in ("x", "y"):
        assert len(view_range[axis]) == 2
        assert view_range[axis][1] > view_range[axis][0]

    crop = data["image_crop"]
    for key in ("x", "y", "width", "height"):
        assert int(crop[key]) == crop[key]
        assert crop[key] >= 0
    assert crop["width"] > 0
    assert crop["height"] > 0
    assert REFERENCE_CROP.is_file(), f"missing ImageView reference crop: {REFERENCE_CROP}"

    if not verify_against_source or not pyqtgraph_runtime_available():
        return

    expected = render_oracle()
    assert data["y_inverted"] == expected["y_inverted"]
    for axis in ("x", "y"):
        for index in (0, 1):
            assert nearly_equal(
                float(view_range[axis][index]),
                float(expected["view_range"][axis][index]),
                tolerance,
            ), f"{axis}[{index}] mismatch"
    for key in ("x", "y", "width", "height"):
        assert int(crop[key]) == int(expected["image_crop"][key])


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="validate the pinned fixture")
    parser.add_argument("--write", action="store_true", help="rewrite the pinned fixture and reference crop")
    parser.add_argument(
        "--require-source",
        action="store_true",
        help="fail if the optional pinned PyQtGraph checkout is absent",
    )
    args = parser.parse_args()

    verify_against_source = args.require_source or not args.check or source_paths_available()
    if verify_against_source:
        require_pinned_sources()
    if args.write:
        FIXTURE.parent.mkdir(parents=True, exist_ok=True)
        FIXTURE.write_text(json.dumps(render_oracle(), indent=2) + "\n", encoding="utf-8")
        write_reference_crop()
        print(f"P409 ImageView view oracle fixture written: {FIXTURE}")
        print(f"P409 ImageView reference crop written: {REFERENCE_CROP}")
        return 0

    if args.check:
        check_fixture(FIXTURE, verify_against_source=verify_against_source)
        print(f"P409 ImageView view oracle fixture ok: {PINNED_REF} {PINNED_COMMIT} ({FIXTURE})")
        return 0

    parser.error("specify --check or --write")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
