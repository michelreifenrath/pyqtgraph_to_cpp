#!/usr/bin/env python3
"""Generate pinned PyQtGraph visual references for parametertree color widgets."""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CHECKOUT = ROOT / "reference" / "pyqtgraph"
OUT_DIR = ROOT / "oracle" / "fixtures" / "parametertree"


def _load_runtime():
    sys.path.insert(0, str(CHECKOUT))
    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    from pyqtgraph.Qt import QtCore, QtGui, QtWidgets
    import pyqtgraph as pg

    return QtCore, QtGui, QtWidgets, pg


def _grab(QtCore, QtWidgets, widget, path: Path, width: int, height: int) -> None:
    widget.resize(width, height)
    widget.show()
    app = QtWidgets.QApplication.instance()
    for _ in range(5):
        app.processEvents(QtCore.QEventLoop.ProcessEventsFlag.AllEvents, 50)
    pixmap = widget.grab()
    path.parent.mkdir(parents=True, exist_ok=True)
    if not pixmap.save(str(path)):
        raise RuntimeError(f"failed to write {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, default=OUT_DIR)
    args = parser.parse_args()

    QtCore, QtGui, QtWidgets, pg = _load_runtime()
    _ = QtGui
    app = QtWidgets.QApplication.instance() or QtWidgets.QApplication([])

    from pyqtgraph.widgets.ColorButton import ColorButton
    from pyqtgraph.widgets.ColorMapButton import ColorMapButton
    from pyqtgraph.widgets.GradientWidget import GradientWidget

    color_button = ColorButton(color=(255, 0, 0))
    color_button.setFlat(True)
    _grab(QtCore, QtWidgets, color_button, args.output_dir / "color_button.reference.png", 120, 30)

    gradient = GradientWidget(orientation="bottom")
    gradient.setMaxDim(35)
    gradient.setMinimumWidth(300)
    gradient.setLength(280.0)
    fixed_map = pg.ColorMap(pos=[0.0, 1.0], color=[(0, 0, 0), (255, 0, 0)], name="fixed")
    gradient.setColorMap(fixed_map)
    _grab(QtCore, QtWidgets, gradient, args.output_dir / "colormap_gradient.reference.png", 300, 35)

    cm = pg.colormap.get("viridis")
    rows = [list(map(int, row[:3])) for row in cm.getLookupTable(0.0, 1.0, 256)]
    (args.output_dir / "viridis_lut.json").write_text(json.dumps(rows), encoding="utf-8")
    cmap_button = ColorMapButton()
    cmap_button.setColorMap("viridis")
    _grab(QtCore, QtWidgets, cmap_button, args.output_dir / "cmaplut_viridis.reference.png", 256, 30)

    _ = app
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
