#!/usr/bin/env python3
"""Generate InteractiveParameter tree oracle from pinned PyQtGraph."""

from __future__ import annotations

import argparse
import importlib.util
import json
import os
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
PINNED_CHECKOUT = ROOT / "reference" / "pyqtgraph"
SOURCE_LOCK = ROOT / "reference" / "source.lock"
INTERACTIVE_EXAMPLE = PINNED_CHECKOUT / "pyqtgraph" / "examples" / "InteractiveParameter.py"
DEFAULT_OUTPUT = ROOT / "oracle" / "fixtures" / "P479" / "interactive_parameter_tree.json"
PINNED_REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"

IN_SCOPE_FUNCTIONS = ("easySample", "stringParams")


def require_pinned_sources() -> None:
    missing = [
        path
        for path in (SOURCE_LOCK, INTERACTIVE_EXAMPLE)
        if not path.exists()
    ]
    if missing:
        names = ", ".join(str(path.relative_to(ROOT)) for path in missing)
        raise SystemExit(f"Pinned PyQtGraph checkout is unavailable; missing {names}")

    lock_text = SOURCE_LOCK.read_text(encoding="utf-8")
    if PINNED_COMMIT not in lock_text or PINNED_REF not in lock_text:
        raise SystemExit(
            "reference/source.lock does not match the InteractiveParameter pinned ref/commit"
        )


def _serialize_parameter(param: Any) -> dict[str, Any]:
    node: dict[str, Any] = {
        "name": param.name(),
        "title": param.title(),
        "type": param.type(),
    }
    opts = param.opts
    if "value" in opts:
        node["value"] = opts["value"]
    if "default" in opts:
        node["default"] = opts["default"]
    if "button" in opts:
        node["button"] = dict(opts["button"])

    children = [_serialize_parameter(child) for child in param]
    if children:
        node["children"] = children
    return node


def _load_pinned_interactive_parameter_host() -> Any:
    """Execute pinned InteractiveParameter.py and return its host Parameter."""
    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    if str(PINNED_CHECKOUT) not in sys.path:
        sys.path.insert(0, str(PINNED_CHECKOUT))

    spec = importlib.util.spec_from_file_location(
        "interactive_parameter_example",
        INTERACTIVE_EXAMPLE,
    )
    if spec is None or spec.loader is None:
        raise SystemExit(f"Failed to load pinned example: {INTERACTIVE_EXAMPLE}")

    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)

    host = getattr(module, "host", None)
    if host is None:
        raise SystemExit("Pinned InteractiveParameter.py did not define host")
    return host


def build_interactive_parameter_tree() -> dict[str, Any]:
    """Mirror in-scope InteractiveParameter.py interactor groups only."""
    host = _load_pinned_interactive_parameter_host()

    child_by_name = {child.name(): child for child in host}
    missing = [name for name in IN_SCOPE_FUNCTIONS if name not in child_by_name]
    if missing:
        raise SystemExit(
            "Pinned InteractiveParameter.py missing in-scope functions: "
            f"{missing}; available={[child.name() for child in host]}"
        )

    in_scope_children = [
        _serialize_parameter(child_by_name[name]) for name in IN_SCOPE_FUNCTIONS
    ]
    tree = {
        "name": host.name(),
        "title": host.title(),
        "type": host.type(),
        "children": in_scope_children,
    }

    return {
        "description": (
            "InteractiveParameter in-scope tree from pinned PyQtGraph "
            "examples/InteractiveParameter.py (easySample, stringParams only). "
            "Never derived from C++ output."
        ),
        "source": "reference/pyqtgraph/pyqtgraph/examples/InteractiveParameter.py",
        "pyqtgraph_ref": PINNED_REF,
        "pinned_commit": PINNED_COMMIT,
        "in_scope_functions": list(IN_SCOPE_FUNCTIONS),
        "tree": tree,
    }


def write_interactive_parameter_oracle(path: Path = DEFAULT_OUTPUT) -> dict[str, Any]:
    fixture = build_interactive_parameter_tree()
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(fixture, indent=2) + "\n", encoding="utf-8")
    return fixture


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Export InteractiveParameter _actiongroup tree oracle "
            "from pinned PyQtGraph interactor construction."
        )
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help="Fixture JSON destination",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    require_pinned_sources()
    write_interactive_parameter_oracle(args.output)
    print(f"wrote InteractiveParameter tree oracle: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
