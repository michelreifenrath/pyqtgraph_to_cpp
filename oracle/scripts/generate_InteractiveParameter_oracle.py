#!/usr/bin/env python3
"""Generate InteractiveParameter tree oracle from pinned PyQtGraph."""

from __future__ import annotations

import argparse
import json
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


def build_interactive_parameter_tree() -> dict[str, Any]:
    """Mirror in-scope InteractiveParameter.py interactor groups only."""
    if str(PINNED_CHECKOUT) not in sys.path:
        sys.path.insert(0, str(PINNED_CHECKOUT))

    from pyqtgraph.parametertree import Interactor, Parameter, RunOptions

    host = Parameter.create(name="Interactive Parameter Use", type="group")
    interactor = Interactor(parent=host, runOptions=RunOptions.ON_CHANGED)

    def easySample(a: int = 5, b: int = 6) -> int:
        return a + b

    def stringParams(a: str = "5", b: str = "6") -> str:
        return a + b

    interactor.interact(easySample)
    interactor.interact(stringParams)

    child_names = [child.name() for child in host]
    if child_names != list(IN_SCOPE_FUNCTIONS):
        raise SystemExit(
            "Unexpected InteractiveParameter host children; "
            f"expected {list(IN_SCOPE_FUNCTIONS)}, got {child_names}"
        )

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
        "tree": _serialize_parameter(host),
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
