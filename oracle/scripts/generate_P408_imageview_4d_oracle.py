#!/usr/bin/env python3
"""Generate/check the P408 ImageView 4D time+RGB oracle fixture from pinned PyQtGraph."""

from __future__ import annotations

import argparse
import ast
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE_LOCK = ROOT / "reference" / "source.lock"
EXAMPLE = ROOT / "reference" / "pyqtgraph" / "pyqtgraph" / "examples" / "ImageView.py"
IMAGEVIEW = ROOT / "reference" / "pyqtgraph" / "pyqtgraph" / "imageview" / "ImageView.py"
FUNCTIONS = ROOT / "reference" / "pyqtgraph" / "pyqtgraph" / "functions.py"
FIXTURE = ROOT / "oracle" / "fixtures" / "P408" / "imageview_4d_oracle.json"
PINNED_REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"
FRAMES = 4
HEIGHT = 3
WIDTH = 3


def source_paths_available() -> bool:
    return all(path.exists() for path in (SOURCE_LOCK, EXAMPLE, IMAGEVIEW, FUNCTIONS))


def require_pinned_sources() -> None:
    missing = [path for path in (SOURCE_LOCK, EXAMPLE, IMAGEVIEW, FUNCTIONS) if not path.exists()]
    if missing:
        names = ", ".join(str(path.relative_to(ROOT)) for path in missing)
        raise SystemExit(f"Pinned PyQtGraph checkout is unavailable; missing {names}")

    lock_text = SOURCE_LOCK.read_text(encoding="utf-8")
    if PINNED_COMMIT not in lock_text or PINNED_REF not in lock_text:
        raise SystemExit("reference/source.lock does not match the P408 pinned PyQtGraph ref/commit")


def load_pinned_gaussian_filter():
    import numpy as np

    def getCupy():
        return None

    source_path = FUNCTIONS.resolve()
    tree = ast.parse(source_path.read_text(encoding="utf-8"), filename=str(source_path))
    func_node = next(
        (
            node
            for node in tree.body
            if isinstance(node, ast.FunctionDef) and node.name == "gaussianFilter"
        ),
        None,
    )
    if func_node is None:
        raise RuntimeError("missing PyQtGraph reference function gaussianFilter")

    module = ast.Module(body=[func_node], type_ignores=[])
    ast.fix_missing_locations(module)
    namespace = {"np": np, "getCupy": getCupy}
    exec(compile(module, str(source_path), "exec"), namespace)
    return namespace["gaussianFilter"]


def build_data() -> tuple[list[float], list[float]]:
    import numpy as np

    gaussian_filter = load_pinned_gaussian_filter()
    np.random.seed(0)
    data_red = np.ones((FRAMES, HEIGHT, WIDTH)) * np.linspace(90, 150, FRAMES)[:, np.newaxis, np.newaxis]
    data_red += gaussian_filter(np.random.normal(size=(HEIGHT, WIDTH)), (5, 5)) * 100
    data_grn = np.ones((FRAMES, HEIGHT, WIDTH)) * np.linspace(90, 180, FRAMES)[:, np.newaxis, np.newaxis]
    data_grn += gaussian_filter(np.random.normal(size=(HEIGHT, WIDTH)), (5, 5)) * 100
    data_blu = np.ones((FRAMES, HEIGHT, WIDTH)) * np.linspace(180, 90, FRAMES)[:, np.newaxis, np.newaxis]
    data_blu += gaussian_filter(np.random.normal(size=(HEIGHT, WIDTH)), (5, 5)) * 100
    data = np.concatenate(
        (
            data_red[:, :, :, np.newaxis],
            data_grn[:, :, :, np.newaxis],
            data_blu[:, :, :, np.newaxis],
        ),
        axis=3,
    )
    xvals = np.linspace(1.0, 3.0, FRAMES)
    return data.astype(np.float64).reshape(-1).tolist(), xvals.astype(np.float64).tolist()


def display_channel(value: float) -> int:
    return int(max(0, min(255, round(value))))


def build_fixture() -> dict[str, object]:
    flat_data, xvals = build_data()
    first_rgb = flat_data[0:3]
    last_offset = ((FRAMES - 1) * HEIGHT * WIDTH + (HEIGHT - 1) * WIDTH + (WIDTH - 1)) * 3
    last_rgb = flat_data[last_offset : last_offset + 3]
    return {
        "issue": "P408",
        "source": {
            "id": PINNED_REF,
            "pinned_commit": PINNED_COMMIT,
            "files": [
                "pyqtgraph/examples/ImageView.py",
                "pyqtgraph/imageview/ImageView.py",
            ],
        },
        "shape": [FRAMES, HEIGHT, WIDTH, 3],
        "xvals": xvals,
        "data": flat_data,
        "probes": {
            "first_frame": {
                "frame": 0,
                "y": 0,
                "x": 0,
                "rgb": first_rgb,
                "display_rgb": [display_channel(value) for value in first_rgb],
            },
            "last_frame": {
                "frame": FRAMES - 1,
                "y": HEIGHT - 1,
                "x": WIDTH - 1,
                "rgb": last_rgb,
                "display_rgb": [display_channel(value) for value in last_rgb],
            },
        },
    }


def probe_rgb(flat_data: list[float], shape: list[int], probe: dict[str, object]) -> list[float]:
    frame = int(probe["frame"])
    y = int(probe["y"])
    x = int(probe["x"])
    channels = int(shape[3])
    base = (((frame * shape[1] + y) * shape[2]) + x) * channels
    return flat_data[base : base + channels]


def check_fixture(path: Path, verify_against_source: bool = True) -> None:
    data = json.loads(path.read_text(encoding="utf-8"))
    source = data.get("source", {})
    assert source.get("id") == PINNED_REF
    assert source.get("pinned_commit") == PINNED_COMMIT
    shape = data["shape"]
    assert shape == [FRAMES, HEIGHT, WIDTH, 3]
    xvals = data["xvals"]
    assert len(xvals) == FRAMES
    assert math.isclose(xvals[0], 1.0)
    assert math.isclose(xvals[-1], 3.0)
    flat_data = data["data"]
    assert len(flat_data) == FRAMES * HEIGHT * WIDTH * 3

    probes = data["probes"]
    for name in ("first_frame", "last_frame"):
        probe = probes[name]
        rgb = probe["rgb"]
        assert rgb == probe_rgb(flat_data, shape, probe)
        assert probe["display_rgb"] == [display_channel(value) for value in rgb]

    if not verify_against_source:
        return

    expected_flat, expected_xvals = build_data()
    assert xvals == expected_xvals
    for name in ("first_frame", "last_frame"):
        probe = probes[name]
        expected_rgb = probe_rgb(expected_flat, shape, probe)
        assert probe["rgb"] == expected_rgb
        assert probe["display_rgb"] == [display_channel(value) for value in expected_rgb]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="validate the pinned fixture")
    parser.add_argument("--write", action="store_true", help="rewrite the pinned fixture")
    parser.add_argument(
        "--require-source",
        action="store_true",
        help="fail if the optional pinned PyQtGraph checkout is absent",
    )
    args = parser.parse_args()

    verify_against_source = args.require_source or not args.check or source_paths_available()
    if verify_against_source:
        require_pinned_sources()
    if args.write:
        FIXTURE.parent.mkdir(parents=True, exist_ok=True)
        FIXTURE.write_text(json.dumps(build_fixture(), indent=2) + "\n", encoding="utf-8")
        print(f"P408 ImageView 4D oracle fixture written: {FIXTURE}")
        return 0

    if args.check:
        check_fixture(FIXTURE, verify_against_source=verify_against_source)
        print(f"P408 ImageView 4D oracle fixture ok: {PINNED_REF} {PINNED_COMMIT} ({FIXTURE})")
        return 0

    parser.error("specify --check or --write")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
