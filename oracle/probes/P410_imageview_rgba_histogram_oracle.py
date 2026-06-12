#!/usr/bin/env python3
"""Pinned ImageView RGBA histogram oracle fixture checker for issue #410."""

from __future__ import annotations

import argparse
import importlib.util
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "oracle" / "scripts" / "generate_P410_imageview_rgba_histogram_oracle.py"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="validate the pinned fixture")
    args = parser.parse_args()
    if not args.check:
        parser.error("only --check is supported")

    spec = importlib.util.spec_from_file_location("generate_P410_imageview_rgba_histogram_oracle", SCRIPT)
    if spec is None or spec.loader is None:
        raise SystemExit(f"failed to load oracle generator: {SCRIPT}")

    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    verify_against_source = module.source_paths_available() and module.pyqtgraph_runtime_available()
    if module.source_paths_available():
        module.require_pinned_sources()
    module.check_fixture(module.FIXTURE, verify_against_source=verify_against_source)
    print(
        "P410 ImageView RGBA histogram oracle fixture ok: "
        f"{module.PINNED_REF} {module.PINNED_COMMIT} ({module.FIXTURE})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
