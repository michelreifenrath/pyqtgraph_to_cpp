#!/usr/bin/env python3
"""Run deterministic interaction scripts against PyQtGraph examples."""

from __future__ import annotations

import argparse
import importlib
import json
import os
import runpy
import sys
import time
from collections.abc import Iterable
from pathlib import Path
from typing import Any, NamedTuple, NoReturn, cast


class InteractionError(RuntimeError):
    """Raised when an interaction script cannot run deterministically."""


class RuntimeModules(NamedTuple):
    QtCore: Any
    QtWidgets: Any
    QtTest: Any


EXAMPLE_RUN_NAME = "__pgoracle_interaction_example__"


class InteractionArgumentParser(argparse.ArgumentParser):
    """ArgumentParser that emits deterministic runner-prefixed errors."""

    def error(self, message: str) -> NoReturn:
        self.exit(2, f"run_interaction_script: {message}\n")


def _pyqtgraph_checkout_root_for_example(example: Path) -> Path | None:
    example_path = example.resolve()
    for ancestor in (example_path.parent, *example_path.parent.parents):
        if ancestor.name != "examples":
            continue
        package_dir = ancestor.parent
        if package_dir.name != "pyqtgraph":
            continue
        checkout_root = package_dir.parent
        if (checkout_root / "pyqtgraph" / "__init__.py").is_file():
            return checkout_root
    return None


def build_parser() -> argparse.ArgumentParser:
    parser = InteractionArgumentParser(
        description="Run a YAML interaction script against a PyQtGraph example."
    )
    parser.add_argument(
        "example",
        type=Path,
        help="path to the PyQtGraph example Python script to execute",
    )
    parser.add_argument(
        "--script",
        type=Path,
        required=True,
        help="YAML interaction script path",
    )
    parser.add_argument(
        "--output",
        type=Path,
        required=True,
        help="JSON interaction run status destination path",
    )
    parser.add_argument(
        "--width",
        type=int,
        default=800,
        help="widget width in pixels (default: 800)",
    )
    parser.add_argument(
        "--height",
        type=int,
        default=600,
        help="widget height in pixels (default: 600)",
    )
    parser.add_argument(
        "--timeout-ms",
        type=int,
        default=250,
        help="bounded time in milliseconds to process Qt events",
    )
    return parser


def _load_runtime() -> RuntimeModules:
    try:
        qt_module = cast(Any, importlib.import_module("pyqtgraph.Qt"))
    except ImportError as exc:
        raise ImportError(str(exc)) from exc
    return RuntimeModules(
        QtCore=qt_module.QtCore,
        QtWidgets=qt_module.QtWidgets,
        QtTest=qt_module.QtTest,
    )


def _is_widget(value: object, QtWidgets: Any) -> bool:
    widget_class = getattr(QtWidgets, "QWidget", None)
    return isinstance(value, widget_class) if isinstance(widget_class, type) else False


def _application(QtWidgets: Any) -> Any:
    application_class = QtWidgets.QApplication
    app = application_class.instance()
    if app is None:
        app = application_class(["run_interaction_script"])
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

    raise InteractionError("example did not create an interactive QWidget")


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
            f"run_interaction_script: missing example file: {args.example}",
            file=sys.stderr,
        )
        return False
    if not args.script.is_file():
        print(
            f"run_interaction_script: missing interaction script file: {args.script}",
            file=sys.stderr,
        )
        return False
    return True


def _yaml_module() -> Any:
    try:
        return importlib.import_module("yaml")
    except ImportError as exc:
        raise InteractionError(f"missing required runtime dependency: {exc}") from exc


def _require_mapping(value: Any, label: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise InteractionError(f"{label} must be a mapping")
    return cast(dict[str, Any], value)


def _positive_int(value: Any, label: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool) or value <= 0:
        raise InteractionError(f"{label} must be a positive integer")
    return value


def _int_value(value: Any, label: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool):
        raise InteractionError(f"{label} must be an integer")
    return value


def _string_value(value: Any, label: str) -> str:
    if not isinstance(value, str) or value == "":
        raise InteractionError(f"{label} must be a non-empty string")
    return value


def _modifiers_value(value: Any, label: str) -> list[str]:
    if value is None:
        return []
    if not isinstance(value, list) or not all(isinstance(item, str) for item in value):
        raise InteractionError(f"{label} must be a list of strings")
    return cast(list[str], value)


def _reject_unknown_fields(step: dict[str, Any], allowed: set[str], label: str) -> None:
    unknown = sorted(set(step) - allowed)
    if unknown:
        raise InteractionError(f"{label} has unknown field(s): {', '.join(unknown)}")


def _normalize_step(step: Any, index: int) -> dict[str, Any]:
    label = f"step {index}"
    step_map = _require_mapping(step, label)
    action = _string_value(step_map.get("action"), f"{label} action")
    if action == "wait":
        _reject_unknown_fields(step_map, {"action", "ms"}, label)
        return {
            "action": action,
            "ms": _positive_int(step_map.get("ms"), f"{label} ms"),
        }
    if action == "mouse_click":
        _reject_unknown_fields(
            step_map, {"action", "x", "y", "button", "modifiers"}, label
        )
        return {
            "action": action,
            "x": _int_value(step_map.get("x"), f"{label} x"),
            "y": _int_value(step_map.get("y"), f"{label} y"),
            "button": _string_value(step_map.get("button", "left"), f"{label} button"),
            "modifiers": _modifiers_value(
                step_map.get("modifiers", []), f"{label} modifiers"
            ),
        }
    if action == "key_click":
        _reject_unknown_fields(step_map, {"action", "key", "modifiers"}, label)
        return {
            "action": action,
            "key": _string_value(step_map.get("key"), f"{label} key"),
            "modifiers": _modifiers_value(
                step_map.get("modifiers", []), f"{label} modifiers"
            ),
        }
    raise InteractionError(f"{label} has unsupported action: {action}")


def load_interaction_script(path: Path) -> list[dict[str, Any]]:
    yaml = _yaml_module()
    try:
        loaded = yaml.safe_load(path.read_text(encoding="utf-8"))
    except Exception as exc:
        raise InteractionError(f"malformed interaction script: {exc}") from exc

    root = _require_mapping(loaded, "interaction script")
    if root.get("version") != 1:
        raise InteractionError("interaction script version must be 1")
    steps = root.get("steps")
    if not isinstance(steps, list):
        raise InteractionError("interaction script steps must be a list")
    return [_normalize_step(step, index) for index, step in enumerate(steps)]


def _qt_namespace(QtCore: Any) -> Any:
    return getattr(QtCore, "Qt", QtCore)


def _constant(namespace: Any, names: Iterable[str], label: str) -> Any:
    for name in names:
        candidate = namespace
        for part in name.split("."):
            if not hasattr(candidate, part):
                candidate = None
                break
            candidate = getattr(candidate, part)
        if candidate is not None:
            return candidate
    raise InteractionError(f"unsupported {label}")


def _button_constant(QtCore: Any, button: str) -> Any:
    names = {
        "left": ("LeftButton", "MouseButton.LeftButton"),
        "right": ("RightButton", "MouseButton.RightButton"),
        "middle": ("MiddleButton", "MouseButton.MiddleButton"),
    }.get(button.lower())
    if names is None:
        raise InteractionError(f"unsupported mouse button: {button}")
    qt = _qt_namespace(QtCore)
    return _constant(qt, names, f"mouse button: {button}")


def _modifier_constant(QtCore: Any, modifier: str) -> Any:
    names = {
        "shift": ("ShiftModifier", "KeyboardModifier.ShiftModifier"),
        "control": ("ControlModifier", "KeyboardModifier.ControlModifier"),
        "ctrl": ("ControlModifier", "KeyboardModifier.ControlModifier"),
        "alt": ("AltModifier", "KeyboardModifier.AltModifier"),
        "meta": ("MetaModifier", "KeyboardModifier.MetaModifier"),
    }.get(modifier.lower())
    if names is None:
        raise InteractionError(f"unsupported keyboard modifier: {modifier}")
    qt = _qt_namespace(QtCore)
    return _constant(qt, names, f"keyboard modifier: {modifier}")


def _no_modifier(QtCore: Any) -> Any:
    qt = _qt_namespace(QtCore)
    return _constant(
        qt, ("NoModifier", "KeyboardModifier.NoModifier"), "keyboard modifier: none"
    )


def _combine_modifiers(QtCore: Any, modifiers: list[str]) -> Any:
    if not modifiers:
        return _no_modifier(QtCore)
    combined = _no_modifier(QtCore)
    for modifier in modifiers:
        combined = combined | _modifier_constant(QtCore, modifier)
    return combined


def _key_constant(QtCore: Any, key: str) -> Any:
    key_names = {
        "enter": "Enter",
        "return": "Return",
        "escape": "Escape",
        "esc": "Escape",
        "left": "Left",
        "right": "Right",
        "up": "Up",
        "down": "Down",
        "plus": "Plus",
        "minus": "Minus",
    }
    qt_key_name = key.upper() if len(key) == 1 else key_names.get(key.lower(), key)
    qt = _qt_namespace(QtCore)
    return _constant(qt, (f"Key_{qt_key_name}",), f"key: {key}")


def _dispatch_step(step: dict[str, Any], widget: Any, runtime: RuntimeModules) -> None:
    qtest = runtime.QtTest.QTest
    action = step["action"]
    if action == "wait":
        qtest.wait(step["ms"])
    elif action == "mouse_click":
        point = runtime.QtCore.QPoint(step["x"], step["y"])
        qtest.mouseClick(
            widget,
            _button_constant(runtime.QtCore, step["button"]),
            _combine_modifiers(runtime.QtCore, step["modifiers"]),
            point,
        )
    elif action == "key_click":
        qtest.keyClick(
            widget,
            _key_constant(runtime.QtCore, step["key"]),
            _combine_modifiers(runtime.QtCore, step["modifiers"]),
        )
    else:  # defensive; load_interaction_script validates this first.
        raise InteractionError(f"unsupported action: {action}")


def run_interactions(args: argparse.Namespace) -> dict[str, Any]:
    steps = load_interaction_script(args.script)
    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    example_dir = str(args.example.resolve().parent)
    checkout_root = _pyqtgraph_checkout_root_for_example(args.example)
    original_sys_path = sys.path[:]
    try:
        if checkout_root is not None:
            sys.path.insert(0, str(checkout_root))
        try:
            runtime = _load_runtime()
        except ImportError as exc:
            raise InteractionError(
                f"missing required runtime dependency: {exc}"
            ) from exc

        app = _application(runtime.QtWidgets)
        original_argv = sys.argv[:]
        try:
            sys.argv = [str(args.example)]
            sys.path.insert(0, example_dir)
            namespace = runpy.run_path(str(args.example), run_name=EXAMPLE_RUN_NAME)
        finally:
            sys.argv = original_argv
    finally:
        sys.path[:] = original_sys_path

    widget = _find_widget(namespace, app, runtime.QtWidgets)
    widget.resize(args.width, args.height)
    widget.show()
    activate_window = getattr(widget, "activateWindow", None)
    if callable(activate_window):
        activate_window()
    set_focus = getattr(widget, "setFocus", None)
    if callable(set_focus):
        set_focus()

    _process_events(app, args.timeout_ms)
    for step in steps:
        _dispatch_step(step, widget, runtime)
        _process_events(app, args.timeout_ms)
    _process_events(app, args.timeout_ms)

    return {
        "status": "ok",
        "example": str(args.example),
        "script": str(args.script),
        "width": args.width,
        "height": args.height,
        "steps_executed": len(steps),
        "actions": [step["action"] for step in steps],
    }


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if not _validate_args(parser, args):
        return 2

    try:
        status = run_interactions(args)
        output_json = json.dumps(status, sort_keys=True, indent=2) + "\n"
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(output_json, encoding="utf-8")
    except InteractionError as exc:
        print(f"run_interaction_script: {exc}", file=sys.stderr)
        return 2
    except Exception as exc:
        print(f"run_interaction_script: {exc}", file=sys.stderr)
        return 2

    print(output_json, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
