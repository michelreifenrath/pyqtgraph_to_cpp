from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
VISUAL_TESTS = ROOT / "tests" / "visual"
if str(VISUAL_TESTS) not in sys.path:
    sys.path.insert(0, str(VISUAL_TESTS))

from imageview_visual import (  # noqa: E402
    REFERENCE,
    assert_image_area_nonempty,
    compare_image_area,
    crop_image_area,
    load_fixture,
    tamper_image_area,
)


def _renderer() -> Path:
    renderer_env = os.environ.get("PG_CPP_P409_VISUAL_RENDERER")
    if not renderer_env:
        pytest.skip(
            "PG_CPP_P409_VISUAL_RENDERER is provided by CTest; run "
            "`ctest --preset visual -L P409 --output-on-failure`"
        )
    renderer = Path(renderer_env)
    assert renderer.is_file(), f"P409 visual renderer does not exist: {renderer}"
    return renderer


def _reports_root(tmp_path: Path) -> Path:
    configured = os.environ.get("PG_P409_VISUAL_REPORTS_ROOT")
    return Path(configured) if configured else tmp_path / "reports" / "visual-diffs"


def _run_renderer(renderer: Path, output: Path) -> dict[str, object]:
    fixture = load_fixture()
    window = fixture["window"]
    result = subprocess.run(
        [
            str(renderer),
            "ImageView",
            "--output",
            str(output),
            "--width",
            str(window["width"]),
            "--height",
            str(window["height"]),
        ],
        cwd=ROOT,
        env={**os.environ, "QT_QPA_PLATFORM": "offscreen"},
        text=True,
        capture_output=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr
    status = json.loads(result.stdout)
    assert status["example"] == "ImageView"
    dimensions = status["dimensions"]
    assert isinstance(dimensions, dict)
    assert dimensions["width"] == window["width"]
    assert dimensions["height"] == window["height"]
    assert status["placeholder"] is False
    assert Path(str(status["output"])).resolve() == output.resolve()
    assert output.is_file()
    return status


def _check_visual_artifacts(
    *, full_frame: Path, reports_root: Path, image_crop: dict[str, object]
) -> dict[str, object]:
    fixture = load_fixture()
    reports_root.mkdir(parents=True, exist_ok=True)
    actual_crop = reports_root / "ImageView" / "image_area.actual.png"
    crop_image_area(full_frame, actual_crop, fixture=fixture, crop=image_crop)
    assert_image_area_nonempty(actual_crop)

    compare_dir = reports_root / "ImageView"
    metrics = compare_image_area(
        REFERENCE,
        actual_crop,
        fixture=fixture,
        work_dir=compare_dir,
    )
    assert metrics["passed"] is True, metrics
    assert metrics["deterministic_verdict"] == "pass", metrics
    assert metrics["geometry_metrics"]["passed"] is True, metrics["geometry_metrics"]
    return metrics


def _assert_visual_gate_fails(actual_crop: Path, tmp_path: Path) -> None:
    work_dir = tmp_path / "negative-compare"
    metrics = compare_image_area(REFERENCE, actual_crop, work_dir=work_dir)
    assert metrics["passed"] is False, metrics


def test_P409_imageview_image_area_matches_reference(tmp_path: Path) -> None:
    renderer = _renderer()
    full_frame = tmp_path / "ImageView.full.png"
    status = _run_renderer(renderer, full_frame)
    dimensions = status["dimensions"]
    assert isinstance(dimensions, dict)
    image_crop = dimensions["image_crop"]
    assert isinstance(image_crop, dict)
    metrics = _check_visual_artifacts(
        full_frame=full_frame,
        reports_root=_reports_root(tmp_path),
        image_crop=image_crop,
    )
    assert metrics["deterministic_verdict"] == "pass"
    assert metrics["geometry_metrics"]["passed"] is True


def test_P409_imageview_tampered_fixture_fails_visual_gate(tmp_path: Path) -> None:
    renderer = _renderer()
    full_frame = tmp_path / "ImageView.full.png"
    status = _run_renderer(renderer, full_frame)
    dimensions = status["dimensions"]
    assert isinstance(dimensions, dict)
    image_crop = dimensions["image_crop"]
    assert isinstance(image_crop, dict)
    actual_crop = tmp_path / "ImageView.image_area.actual.png"
    crop_image_area(full_frame, actual_crop, crop=image_crop)
    tampered = tmp_path / "ImageView.image_area.tampered.png"
    tamper_image_area(actual_crop, tampered)
    _assert_visual_gate_fails(tampered, tmp_path)
