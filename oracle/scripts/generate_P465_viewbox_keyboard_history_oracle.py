#!/usr/bin/env python3
"""Generate/check ViewBox keyboard zoom history oracle from pinned PyQtGraph."""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
SOURCE_LOCK = ROOT / "reference" / "source.lock"
VIEWBOX = ROOT / "reference" / "pyqtgraph" / "pyqtgraph" / "graphicsItems" / "ViewBox" / "ViewBox.py"
FIXTURE = ROOT / "oracle" / "fixtures" / "interactions" / "ViewBox_keyboard_history.json"
PINNED_REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"
VIEWBOX_SIZE = (200.0, 100.0)
INITIAL_RANGE = {"x": [0.0, 10.0], "y": [0.0, 10.0]}


def require_pinned_sources() -> None:
    missing = [path for path in (SOURCE_LOCK, VIEWBOX) if not path.exists()]
    if missing:
        names = ", ".join(str(path.relative_to(ROOT)) for path in missing)
        raise SystemExit(f"Pinned PyQtGraph checkout is unavailable; missing {names}")

    lock_text = SOURCE_LOCK.read_text(encoding="utf-8")
    if PINNED_COMMIT not in lock_text or PINNED_REF not in lock_text:
        raise SystemExit("reference/source.lock does not match the P465 pinned PyQtGraph ref/commit")


def range_dict(view_box: Any) -> dict[str, list[float]]:
    x_range, y_range = view_box.viewRange()
    return {"x": [float(x_range[0]), float(x_range[1])], "y": [float(y_range[0]), float(y_range[1])]}


def build_oracle() -> dict[str, Any]:
    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    pinned_checkout = ROOT / "reference" / "pyqtgraph"
    if str(pinned_checkout) not in sys.path:
        sys.path.insert(0, str(pinned_checkout))

    import pyqtgraph as pg
    from pyqtgraph.Qt import QtCore, QtGui, QtWidgets
    from pyqtgraph.Point import Point

    pg.mkQApp("P465 ViewBox keyboard history oracle")
    scene = QtWidgets.QGraphicsScene()
    view_box = pg.ViewBox()
    scene.addItem(view_box)
    view_box.resize(*VIEWBOX_SIZE)
    view_box.setDefaultPadding(0.0)
    view_box.setRange(
        xRange=INITIAL_RANGE["x"],
        yRange=INITIAL_RANGE["y"],
        padding=0.0,
    )
    view_box.setMouseMode(pg.ViewBox.RectMode)

    def do_rect_zoom(start: tuple[float, float], end: tuple[float, float]) -> None:
        view_box.updateMatrix()
        ax = QtCore.QRectF(view_box.mapToView(Point(*start)), view_box.mapToView(Point(*end))).normalized()
        view_box.showAxRect(ax)
        view_box.axHistoryPointer += 1
        view_box.axHistory = view_box.axHistory[: view_box.axHistoryPointer] + [ax]

    def send_key(text: str | None = None, key: QtCore.Qt.Key | None = None) -> None:
        event = QtGui.QKeyEvent(
            QtCore.QEvent.Type.KeyPress,
            key if key is not None else QtCore.Qt.Key.Key_unknown,
            QtCore.Qt.KeyboardModifier.NoModifier,
            text or "",
        )
        view_box.keyPressEvent(event)

    steps: list[dict[str, Any]] = []
    steps.append({"action": "initial", "range": range_dict(view_box)})

    do_rect_zoom((25.0, 25.0), (175.0, 75.0))
    steps.append({"action": "rect_zoom_1", "range": range_dict(view_box)})

    do_rect_zoom((50.0, 30.0), (150.0, 70.0))
    steps.append({"action": "rect_zoom_2", "range": range_dict(view_box)})

    send_key(text="-")
    steps.append({"action": "key_minus", "range": range_dict(view_box)})

    send_key(text="+")
    steps.append({"action": "key_plus", "range": range_dict(view_box)})

    send_key(text="=")
    steps.append({"action": "key_equals", "range": range_dict(view_box)})

    send_key(key=QtCore.Qt.Key.Key_Backspace)
    steps.append({"action": "key_backspace", "range": range_dict(view_box)})

    return {
        "description": (
            "Deterministic ViewBox rect-zoom keyboard history from pinned PyQtGraph "
            "ViewBox.py keyPressEvent/scaleHistory at a20028b."
        ),
        "source": "reference/pyqtgraph/pyqtgraph/graphicsItems/ViewBox/ViewBox.py",
        "pyqtgraph_ref": PINNED_REF,
        "pinned_commit": PINNED_COMMIT,
        "viewbox_size": list(VIEWBOX_SIZE),
        "initial": INITIAL_RANGE,
        "mouse_mode": "RectMode",
        "steps": steps,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="validate the pinned fixture")
    parser.add_argument("--write", action="store_true", help="regenerate the fixture JSON")
    args = parser.parse_args()

    require_pinned_sources()

    if args.write:
        fixture = build_oracle()
        FIXTURE.parent.mkdir(parents=True, exist_ok=True)
        FIXTURE.write_text(json.dumps(fixture, indent=2) + "\n", encoding="utf-8")
        print(f"Wrote {FIXTURE}")
        return 0

    if not FIXTURE.exists():
        raise SystemExit(f"Fixture missing: {FIXTURE}; run with --write")

    expected = build_oracle()
    actual = json.loads(FIXTURE.read_text(encoding="utf-8"))
    if actual != expected:
        if args.check:
            raise SystemExit(f"Fixture drift: {FIXTURE}")
        print("Fixture differs from pinned oracle; use --write to refresh", file=sys.stderr)
        return 1

    print(f"P465 ViewBox keyboard history oracle ok: {PINNED_REF} {PINNED_COMMIT} ({FIXTURE})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
