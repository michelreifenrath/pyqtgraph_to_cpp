#!/usr/bin/env python3
"""Generate the Plotting visual reference from pinned PyQtGraph with seeded data."""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path
from typing import Any

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
PINNED_CHECKOUT = ROOT / "reference" / "pyqtgraph"
SOURCE_LOCK = ROOT / "reference" / "source.lock"
PLOTTING_EXAMPLE = PINNED_CHECKOUT / "pyqtgraph" / "examples" / "Plotting.py"
PLOTTING_DATA_FIXTURE = ROOT / "oracle" / "fixtures" / "P372" / "plotting_data.json"
DEFAULT_OUTPUT = ROOT / "oracle" / "fixtures" / "screenshots" / "Plotting.reference.png"
CROP_DIR = ROOT / "oracle" / "fixtures" / "P372" / "screenshots"
PINNED_REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"
PLOTTING_SEED = 0x504C5454
IMAGE_WIDTH = 1000
IMAGE_HEIGHT = 600


def require_pinned_sources() -> None:
    missing = [
        path
        for path in (SOURCE_LOCK, PLOTTING_EXAMPLE)
        if not path.exists()
    ]
    if missing:
        names = ", ".join(str(path.relative_to(ROOT)) for path in missing)
        raise SystemExit(f"Pinned PyQtGraph checkout is unavailable; missing {names}")

    lock_text = SOURCE_LOCK.read_text(encoding="utf-8")
    if PINNED_COMMIT not in lock_text or PINNED_REF not in lock_text:
        raise SystemExit(
            "reference/source.lock does not match the Plotting pinned ref/commit"
        )


def _load_pyqtgraph():
    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    if str(PINNED_CHECKOUT) not in sys.path:
        sys.path.insert(0, str(PINNED_CHECKOUT))
    import pyqtgraph as pg
    from pyqtgraph.Qt import QtCore

    return pg, QtCore


def build_plotting_data() -> dict[str, Any]:
    """Build Plotting arrays using the pinned upstream call order and a fixed NumPy seed."""
    np.random.seed(PLOTTING_SEED)

    p1_y = np.random.normal(size=100)
    p2_red = np.random.normal(size=100)
    p2_green = np.random.normal(size=110) + 5.0
    p2_blue = np.random.normal(size=120) + 10.0
    p3_y = np.random.normal(size=100)

    p4_x = np.cos(np.linspace(0, 2 * np.pi, 1000))
    p4_y = np.sin(np.linspace(0, 4 * np.pi, 1000))

    x5 = np.random.normal(size=1000) * 1.0e-5
    y5 = x5 * 1000.0 + 0.005 * np.random.normal(size=1000)
    y5 -= y5.min() - 1.0
    mask = x5 > 1.0e-15
    p5_x = x5[mask]
    p5_y = y5[mask]

    p6_rows = np.random.normal(size=(10, 1000))

    p7_y = np.sin(np.linspace(0, 10, 1000)) + np.random.normal(size=1000, scale=0.1)

    x2 = np.linspace(-100.0, 100.0, 1000)
    sinc_data = np.sin(x2) / x2

    return {
        "description": (
            "Deterministic Plotting data arrays from pinned PyQtGraph examples/Plotting.py "
            f"(np.random.seed({PLOTTING_SEED})). Used by the visual oracle and C++ test-mode "
            "fixture loader; never derived from C++ output."
        ),
        "source": "reference/pyqtgraph/pyqtgraph/examples/Plotting.py",
        "pyqtgraph_ref": PINNED_REF,
        "pinned_commit": PINNED_COMMIT,
        "seed": PLOTTING_SEED,
        "arrays": {
            "p1_y": p1_y.tolist(),
            "p2_red": p2_red.tolist(),
            "p2_green": p2_green.tolist(),
            "p2_blue": p2_blue.tolist(),
            "p3_y": p3_y.tolist(),
            "p4_x": p4_x.tolist(),
            "p4_y": p4_y.tolist(),
            "p5_x": p5_x.tolist(),
            "p5_y": p5_y.tolist(),
            "p6_rows": p6_rows.tolist(),
            "p6_initial_row": p6_rows[0].tolist(),
            "p7_y": p7_y.tolist(),
            "sinc_data": sinc_data.tolist(),
        },
        "region_initial": [400.0, 700.0],
    }


def write_plotting_data_fixture(path: Path = PLOTTING_DATA_FIXTURE) -> dict[str, Any]:
    fixture = build_plotting_data()
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(fixture, indent=2) + "\n", encoding="utf-8")
    return fixture


def load_plotting_data_fixture(fixture_path: Path = PLOTTING_DATA_FIXTURE) -> dict[str, Any]:
    if not fixture_path.is_file():
        raise SystemExit(
            f"Missing Plotting data fixture: {fixture_path}. "
            "Regenerate with --export-data-fixture from pinned PyQtGraph."
        )
    fixture = json.loads(fixture_path.read_text(encoding="utf-8"))
    if fixture.get("pinned_commit") != PINNED_COMMIT:
        raise SystemExit(
            f"{fixture_path} pinned_commit does not match Plotting visual reference commit"
        )
    return fixture


def render_deterministic_plotting(
    output: Path,
    *,
    crop_dir: Path | None,
    fixture_path: Path = PLOTTING_DATA_FIXTURE,
) -> None:
    pg, QtCore = _load_pyqtgraph()
    fixture = load_plotting_data_fixture(fixture_path)
    arrays = fixture["arrays"]

    app = pg.mkQApp("Plotting visual reference")
    pg.setConfigOptions(antialias=True)
    win = pg.GraphicsLayoutWidget(show=True, title="Basic plotting examples")
    win.resize(IMAGE_WIDTH, IMAGE_HEIGHT)
    win.setWindowTitle("pyqtgraph example: Plotting")

    p1 = win.addPlot(title="Basic array plotting", y=np.asarray(arrays["p1_y"]))

    p2 = win.addPlot(title="Multiple curves")
    p2.plot(np.asarray(arrays["p2_red"]), pen=(255, 0, 0), name="Red curve")
    p2.plot(np.asarray(arrays["p2_green"]), pen=(0, 255, 0), name="Green curve")
    p2.plot(np.asarray(arrays["p2_blue"]), pen=(0, 0, 255), name="Blue curve")

    p3 = win.addPlot(title="Drawing with points")
    p3.plot(
        np.asarray(arrays["p3_y"]),
        pen=(200, 200, 200),
        symbolBrush=(255, 0, 0),
        symbolPen="w",
    )

    win.nextRow()

    p4 = win.addPlot(title="Parametric, grid enabled")
    p4.plot(np.asarray(arrays["p4_x"]), np.asarray(arrays["p4_y"]))
    p4.showGrid(x=True, y=True)

    p5 = win.addPlot(title="Scatter plot, axis labels, log scale")
    p5.plot(
        np.asarray(arrays["p5_x"]),
        np.asarray(arrays["p5_y"]),
        pen=None,
        symbol="t",
        symbolPen=None,
        symbolSize=10,
        symbolBrush=(100, 100, 255, 50),
    )
    p5.setLabel("left", "Y Axis", units="A")
    p5.setLabel("bottom", "Y Axis", units="s")
    p5.setLogMode(x=True, y=False)

    p6 = win.addPlot(title="Updating plot")
    curve = p6.plot(pen="y")
    update_data = np.asarray(arrays["p6_rows"])
    update_ptr = 0

    def update() -> None:
        nonlocal update_ptr
        curve.setData(update_data[update_ptr % len(update_data)])
        if update_ptr == 0:
            p6.enableAutoRange("xy", False)
        update_ptr += 1

    update()

    win.nextRow()

    p7 = win.addPlot(title="Filled plot, axis disabled")
    p7.plot(
        np.asarray(arrays["p7_y"]),
        fillLevel=-0.3,
        brush=(50, 50, 200, 100),
    )
    p7.showAxis("bottom", False)

    sinc_data = np.asarray(arrays["sinc_data"])
    p8 = win.addPlot(title="Region Selection")
    p8.plot(sinc_data, pen=(255, 255, 255, 200))
    region = pg.LinearRegionItem(list(fixture["region_initial"]))
    region.setZValue(-10)
    p8.addItem(region)

    p9 = win.addPlot(title="Zoom on selected region")
    p9.plot(sinc_data)

    def update_plot() -> None:
        p9.setXRange(*region.getRegion(), padding=0)

    def update_region() -> None:
        region.setRegion(p9.getViewBox().viewRange()[0])

    region.sigRegionChanged.connect(update_plot)
    p9.sigXRangeChanged.connect(update_region)
    update_plot()

    for _ in range(3):
        app.processEvents()

    output.parent.mkdir(parents=True, exist_ok=True)
    pixmap = win.grab()
    if not pixmap.save(str(output), "PNG"):
        raise SystemExit(f"failed to save Plotting reference screenshot: {output}")

    if crop_dir is not None:
        tests_visual = ROOT / "tests" / "visual"
        if str(tests_visual) not in sys.path:
            sys.path.insert(0, str(tests_visual))
        from plotting_subplot_visual import SUBPLOT_CELLS, write_subplot_png

        crop_dir.mkdir(parents=True, exist_ok=True)
        for cell in SUBPLOT_CELLS:
            write_subplot_png(
                output,
                crop_dir / f"Plotting.{cell.name}.reference.png",
                col=cell.col,
                row=cell.row,
            )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Render the Plotting visual reference from pinned PyQtGraph while "
            "injecting seeded Plotting data exported from the pinned upstream example."
        )
    )
    parser.add_argument(
        "--export-data-fixture",
        action="store_true",
        help="Write oracle/fixtures/P372/plotting_data.json from pinned PyQtGraph seed logic",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help="Full-frame PNG destination",
    )
    parser.add_argument(
        "--crop-dir",
        type=Path,
        default=CROP_DIR,
        help="Directory for per-subplot reference crops",
    )
    parser.add_argument(
        "--no-crops",
        action="store_true",
        help="Skip writing per-subplot reference crops",
    )
    parser.add_argument(
        "--data-fixture",
        type=Path,
        default=PLOTTING_DATA_FIXTURE,
        help="Plotting data arrays exported from pinned PyQtGraph seed logic",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    require_pinned_sources()
    if args.export_data_fixture:
        write_plotting_data_fixture(args.data_fixture)
        print(f"wrote Plotting data fixture: {args.data_fixture}")
    render_deterministic_plotting(
        args.output,
        crop_dir=None if args.no_crops else args.crop_dir,
        fixture_path=args.data_fixture,
    )
    print(f"wrote Plotting reference: {args.output}")
    if not args.no_crops:
        print(f"wrote subplot crops: {args.crop_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
