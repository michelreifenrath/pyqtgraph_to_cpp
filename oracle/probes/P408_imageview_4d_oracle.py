#!/usr/bin/env python3
"""Pinned ImageView 4D time+RGB oracle fixture checker for issue #408."""

from __future__ import annotations

import argparse
import importlib.util
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "oracle" / "scripts" / "generate_P408_imageview_4d_oracle.py"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="validate the pinned fixture")
    args = parser.parse_args()
    if not args.check:
        parser.error("only --check is supported")

    spec = importlib.util.spec_from_file_location("generate_P408_imageview_4d_oracle", SCRIPT)
    if spec is None or spec.loader is None:
        raise SystemExit(f"failed to load oracle generator: {SCRIPT}")

    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    module.require_pinned_sources()
    module.check_fixture(module.FIXTURE)
    print(
        "P408 ImageView 4D oracle fixture ok: "
        f"{module.PINNED_REF} {module.PINNED_COMMIT} ({module.FIXTURE})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
