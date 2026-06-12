#!/usr/bin/env python3
"""Generate/check the P410 ImageView RGBA histogram oracle from pinned PyQtGraph."""

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
HISTOGRAM = ROOT / "reference" / "pyqtgraph" / "pyqtgraph" / "graphicsItems" / "HistogramLUTItem.py"
FIXTURE = ROOT / "oracle" / "fixtures" / "P410" / "imageview_rgba_histogram_oracle.json"
PINNED_REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"
FRAMES = 4
HEIGHT = 3
WIDTH = 3
CHANNELS = 3
LABEL = "Histogram label goes here"


def source_paths_available() -> bool:
    return all(path.exists() for path in (SOURCE_LOCK, EXAMPLE, IMAGEVIEW, HISTOGRAM))


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
    missing = [path for path in (SOURCE_LOCK, EXAMPLE, IMAGEVIEW, HISTOGRAM) if not path.exists()]
    if missing:
        names = ", ".join(str(path.relative_to(ROOT)) for path in missing)
        raise SystemExit(f"Pinned PyQtGraph checkout is unavailable; missing {names}")

    lock_text = SOURCE_LOCK.read_text(encoding="utf-8")
    if PINNED_COMMIT not in lock_text or PINNED_REF not in lock_text:
        raise SystemExit("reference/source.lock does not match the P410 pinned PyQtGraph ref/commit")


def build_example_data() -> tuple[Any, Any]:
    import numpy as np

    data = np.zeros((FRAMES, HEIGHT, WIDTH, CHANNELS), dtype=np.float32)
    for frame in range(FRAMES):
        data[frame, :, :, 0] = 90.0 + 60.0 * frame / (FRAMES - 1)
        data[frame, :, :, 1] = 90.0 + 90.0 * frame / (FRAMES - 1)
        data[frame, :, :, 2] = 180.0 - 90.0 * frame / (FRAMES - 1)
    xvals = np.linspace(1.0, 3.0, FRAMES)
    return data, xvals


def display_channel(value: float, minimum: float, maximum: float) -> int:
    span = maximum - minimum
    if span == 0.0:
        return int(max(0, min(255, round(value))))
    scaled = (value - minimum) / span
    return int(max(0, min(255, round(scaled * 255.0))))


def render_oracle() -> dict[str, Any]:
    import numpy as np

    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    pinned_checkout = ROOT / "reference" / "pyqtgraph"
    if str(pinned_checkout) not in sys.path:
        sys.path.insert(0, str(pinned_checkout))
    import pyqtgraph as pg
    from pyqtgraph.Qt import QtWidgets

    data, xvals = build_example_data()
    pg.setConfigOptions(imageAxisOrder="row-major")
    app = pg.mkQApp("P410 ImageView RGBA histogram oracle")
    win = QtWidgets.QMainWindow()
    win.resize(800, 800)
    imv = pg.ImageView(levelMode="rgba")
    win.setCentralWidget(imv)
    imv.setHistogramLabel(LABEL)
    imv.setImage(data, xvals=xvals)
    for _ in range(5):
        app.processEvents()

    histogram = imv.ui.histogram.item
    levels = histogram.getLevels()
    regions = []
    for index in range(1, CHANNELS + 1):
        region = histogram.regions[index]
        span = region.span
        regions.append(
            {
                "visible": bool(region.isVisible()),
                "span": [float(span[0]), float(span[1])],
                "levels": [float(levels[index - 1][0]), float(levels[index - 1][1])],
            }
        )

    shifted_levels = [list(level) for level in levels]
    shifted_levels[0] = [levels[0][0] + 10.0, levels[0][1] - 10.0]
    histogram.setLevels(rgba=shifted_levels)
    for _ in range(5):
        app.processEvents()

    image_levels = imv.imageItem.getLevels()
    pixel = imv.imageItem.image[0, 0].tolist()
    display_rgb = [
        display_channel(pixel[0], image_levels[0][0], image_levels[0][1]),
        display_channel(pixel[1], image_levels[1][0], image_levels[1][1]),
        display_channel(pixel[2], image_levels[2][0], image_levels[2][1]),
    ]

    return {
        "issue": "P410",
        "source": {
            "id": PINNED_REF,
            "pinned_commit": PINNED_COMMIT,
            "files": [
                "pyqtgraph/examples/ImageView.py",
                "pyqtgraph/imageview/ImageView.py",
                "pyqtgraph/graphicsItems/HistogramLUTItem.py",
            ],
        },
        "level_mode": "rgba",
        "histogram_label": LABEL,
        "shape": [FRAMES, HEIGHT, WIDTH, CHANNELS],
        "xvals": xvals.astype(float).tolist(),
        "data": data.astype(np.float32).reshape(-1).tolist(),
        "regions": regions,
        "level_shift": {
            "rgba": [[float(pair[0]), float(pair[1])] for pair in shifted_levels[:CHANNELS]],
        },
        "render_probe": {
            "frame": 0,
            "y": 0,
            "x": 0,
            "source_rgb": pixel,
            "image_levels": [[float(pair[0]), float(pair[1])] for pair in image_levels],
            "display_rgb": display_rgb,
        },
        "gradient_hidden": not histogram.gradient.isVisible(),
        "mono_plot_hidden": not histogram.plots[0].isVisible(),
        "rgb_plots_visible": [bool(histogram.plots[index].isVisible()) for index in range(1, CHANNELS + 1)],
    }


def nearly_equal(lhs: float, rhs: float, tolerance: float = 1.0e-6) -> bool:
    return abs(lhs - rhs) <= tolerance


def check_fixture(path: Path, verify_against_source: bool = True) -> None:
    data = json.loads(path.read_text(encoding="utf-8"))
    source = data.get("source", {})
    assert source.get("id") == PINNED_REF
    assert source.get("pinned_commit") == PINNED_COMMIT
    assert data["level_mode"] == "rgba"
    assert data["histogram_label"] == LABEL
    assert data["shape"] == [FRAMES, HEIGHT, WIDTH, CHANNELS]
    assert len(data["regions"]) == CHANNELS
    for index, region in enumerate(data["regions"]):
        assert region["visible"] is True
        expected_span = [index / CHANNELS, (index + 1) / CHANNELS]
        assert nearly_equal(region["span"][0], expected_span[0])
        assert nearly_equal(region["span"][1], expected_span[1])
    assert data["gradient_hidden"] is True
    assert data["mono_plot_hidden"] is True
    assert data["rgb_plots_visible"] == [True, True, True]

    if verify_against_source and pyqtgraph_runtime_available():
        rendered = render_oracle()
        for key in ("level_mode", "histogram_label", "gradient_hidden", "mono_plot_hidden", "rgb_plots_visible"):
            assert data[key] == rendered[key], key
        for index, region in enumerate(data["regions"]):
            for field in ("visible", "span"):
                assert region[field] == rendered["regions"][index][field], f"regions[{index}].{field}"
            for axis in (0, 1):
                assert nearly_equal(region["levels"][axis], rendered["regions"][index]["levels"][axis], 1.0)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="write the fixture")
    parser.add_argument("--check", action="store_true", help="validate the fixture")
    args = parser.parse_args()
    if not args.write and not args.check:
        parser.error("specify --write or --check")

    require_pinned_sources()
    if args.write:
        FIXTURE.parent.mkdir(parents=True, exist_ok=True)
        FIXTURE.write_text(json.dumps(render_oracle(), indent=2) + "\n", encoding="utf-8")
        print(f"wrote {FIXTURE}")
        return 0

    verify_against_source = source_paths_available() and pyqtgraph_runtime_available()
    check_fixture(FIXTURE, verify_against_source=verify_against_source)
    print(f"P410 ImageView RGBA histogram oracle fixture ok: {PINNED_REF} {PINNED_COMMIT} ({FIXTURE})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
