#!/usr/bin/env python3
"""Render a PyQtGraph example script to a PNG screenshot."""

from __future__ import annotations

import argparse
import importlib
import os
import runpy
import sys
import time
from collections.abc import Iterable
from pathlib import Path
from typing import Any, NamedTuple, cast


class RenderError(RuntimeError):
    """Raised when a screenshot cannot be rendered deterministically."""


class RuntimeModules(NamedTuple):
    QtCore: Any
    QtWidgets: Any


EXAMPLE_RUN_NAME = "__pgoracle_example__"


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Render a PyQtGraph example Python script to a PNG screenshot."
    )
    parser.add_argument(
        "example",
        type=Path,
        help="path to the PyQtGraph example Python script to execute",
    )
    parser.add_argument(
        "--output",
        type=Path,
        required=True,
        help="PNG screenshot destination path",
    )
    parser.add_argument(
        "--width",
        type=int,
        default=800,
        help="screenshot width in pixels (default: 800)",
    )
    parser.add_argument(
        "--height",
        type=int,
        default=600,
        help="screenshot height in pixels (default: 600)",
    )
    parser.add_argument(
        "--timeout-ms",
        type=int,
        default=250,
        help="bounded time in milliseconds to process Qt events before capture",
    )
    return parser


def _load_runtime() -> RuntimeModules:
    try:
        qt_module = cast(Any, importlib.import_module("pyqtgraph.Qt"))
    except ImportError as exc:
        raise ImportError(str(exc)) from exc
    return RuntimeModules(QtCore=qt_module.QtCore, QtWidgets=qt_module.QtWidgets)


def _is_widget(value: object, QtWidgets: Any) -> bool:
    widget_class = getattr(QtWidgets, "QWidget", None)
    return isinstance(value, widget_class) if isinstance(widget_class, type) else False


def _application(QtWidgets: Any) -> Any:
    application_class = QtWidgets.QApplication
    app = application_class.instance()
    if app is None:
        app = application_class(["render_pyqtgraph_example"])
    return app


def _find_widget(namespace: dict[str, Any], app: Any, QtWidgets: Any) -> Any:
    for name in ("widget", "win", "window"):
        candidate = namespace.get(name)
        if _is_widget(candidate, QtWidgets):
            return candidate

    top_level_widgets = getattr(app, "topLevelWidgets", None)
    if callable(top_level_widgets):
        candidates = cast(Iterable[object], top_level_widgets())
        for candidate in candidates:
            if _is_widget(candidate, QtWidgets):
                return candidate

    raise RenderError("example did not create a capturable QWidget")


def _process_events(app: Any, timeout_ms: int) -> None:
    deadline = time.monotonic() + (timeout_ms / 1000.0)
    while True:
        process_events = getattr(app, "processEvents", None)
        if callable(process_events):
            process_events()
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break
        time.sleep(min(0.01, remaining))


def _validate_args(parser: argparse.ArgumentParser, args: argparse.Namespace) -> bool:
    if args.width <= 0 or args.height <= 0:
        parser.error("positive width/height are required")
    if args.timeout_ms <= 0:
        parser.error("positive --timeout-ms is required")
    if not args.example.is_file():
        print(
            f"render_pyqtgraph_example: missing example file: {args.example}",
            file=sys.stderr,
        )
        return False
    return True


def render(args: argparse.Namespace) -> None:
    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    try:
        runtime = _load_runtime()
    except ImportError as exc:
        raise RenderError(f"missing required runtime dependency: {exc}") from exc

    app = _application(runtime.QtWidgets)
    namespace = runpy.run_path(str(args.example), run_name=EXAMPLE_RUN_NAME)
    widget = _find_widget(namespace, app, runtime.QtWidgets)
    widget.resize(args.width, args.height)
    widget.show()
    _process_events(app, args.timeout_ms)

    pixmap = widget.grab()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if not pixmap.save(str(args.output), "PNG"):
        raise RenderError(f"failed to save PNG screenshot: {args.output}")


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if not _validate_args(parser, args):
        return 2

    try:
        render(args)
    except RenderError as exc:
        print(f"render_pyqtgraph_example: {exc}", file=sys.stderr)
        return 2
    except Exception as exc:
        print(f"render_pyqtgraph_example: {exc}", file=sys.stderr)
        return 2

    print(f"wrote screenshot: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
