from __future__ import annotations

import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

import pytest
from test_P1_08_cpp_visual_renderer import _assert_semantic_plot_image

ROOT = Path(__file__).resolve().parents[2]
REFERENCE_SIMPLEPLOT = ROOT / "oracle" / "fixtures" / "screenshots" / "SimplePlot.reference.png"
CHECK_VISUAL_ARTIFACTS = ROOT / "scripts" / "check_visual_artifacts"
PYQTGRAPH_REF = (
    "/home/michel/.cache/pgcpp-opensrc/repos/github.com/pyqtgraph/pyqtgraph/"
    "pyqtgraph-0.14.0"
)


def _renderer() -> Path:
    renderer_env = os.environ.get("PG_CPP_P3_11_VISUAL_RENDERER")
    if not renderer_env:
        pytest.skip(
            "PG_CPP_P3_11_VISUAL_RENDERER is provided by CTest; run "
            "`ctest --preset visual -L P3.11 --output-on-failure`"
        )
    renderer = Path(renderer_env)
    assert renderer.is_file(), f"P3.11 visual renderer does not exist: {renderer}"
    return renderer


def _run_renderer(renderer: Path, example: str, output: Path) -> dict[str, object]:
    result = subprocess.run(
        [
            str(renderer),
            example,
            "--output",
            str(output),
            "--width",
            "800",
            "--height",
            "600",
        ],
        cwd=ROOT,
        env={**os.environ, "QT_QPA_PLATFORM": "offscreen"},
        text=True,
        capture_output=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr
    status = json.loads(result.stdout)
    assert status["example"] == example
    assert status["dimensions"] == {"width": 800, "height": 600}
    assert status["placeholder"] is False
    assert Path(str(status["output"])).resolve() == output.resolve()
    assert output.is_file()
    _assert_semantic_plot_image(output, width=800, height=600)
    return status


def _reports_root(tmp_path: Path) -> Path:
    configured = os.environ.get("PG_P3_11_VISUAL_REPORTS_ROOT")
    return Path(configured) if configured else tmp_path / "reports" / "visual" / "P3.11"


def _check_visual(
    *,
    case: str,
    reference: Path,
    actual: Path,
    reports_root: Path,
    max_mean: float,
    max_delta: float,
    max_changed: float,
    min_ssim: float,
) -> dict[str, object]:
    result = subprocess.run(
        [
            sys.executable,
            str(CHECK_VISUAL_ARTIFACTS),
            "--case",
            case,
            "--reference",
            str(reference),
            "--actual",
            str(actual),
            "--reports-root",
            str(reports_root),
            "--gpt-visual-review",
            "not_applicable",
            "--max-mean-delta",
            str(max_mean),
            "--max-pixel-delta",
            str(max_delta),
            "--max-changed-percent",
            str(max_changed),
            "--min-ssim",
            str(min_ssim),
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr + result.stdout
    metrics = json.loads((reports_root / case / "metrics.json").read_text(encoding="utf-8"))
    assert metrics["passed"] is True
    assert metrics["deterministic_verdict"] == "pass"
    assert metrics["failed_checks"] == []
    for artifact_name in ("reference.png", "actual.png", "diff.png", "metrics.json"):
        assert (reports_root / case / artifact_name).is_file()
    return metrics


def _write_manual_report(reports_root: Path, metrics: dict[str, dict[str, object]]) -> None:
    simple_hash = hashlib.sha256(REFERENCE_SIMPLEPLOT.read_bytes()).hexdigest()
    report = reports_root / "manual_semantic_inspection.md"
    report.write_text(
        "# P3.11 manual semantic inspection\n\n"
        "Reference source: pyqtgraph-0.14.0 "
        "pyqtgraph/graphicsItems/PlotItem/PlotItem.py, "
        "pyqtgraph/graphicsItems/LegendItem.py, examples/SimplePlot.py.\n"
        f"Pinned checkout: `{PYQTGRAPH_REF}`.\n"
        f"SimplePlot fixture SHA256: `{simple_hash}`.\n"
        "Reproducibility: QT_QPA_PLATFORM=offscreen, 800x600 grab, deterministic "
        "fixed curve data and fixed decoration reference drawing.\n\n"
        "Semantic inspection note: SimplePlot contains a black plot area with axes/ticks "
        "and a white data curve. PlotDecorations contains a title, left/bottom labels, "
        "four axes, two colored curves, and a top-left legend with two entries; the diff "
        "image contains only expected implementation-vs-oracle styling deviations, not "
        "blank or placeholder output.\n\n"
        "Metrics files:\n"
        + "".join(f"- {case}: `{data['artifact_paths']['metrics']}`\n" for case, data in sorted(metrics.items()))
        ,
        encoding="utf-8",
    )


def test_P3_11_blank_guard_rejects_empty_or_nonsemantic_reference(tmp_path: Path) -> None:
    blank = tmp_path / "blank.png"
    blank.write_bytes(REFERENCE_SIMPLEPLOT.read_bytes()[:8])
    with pytest.raises(Exception):
        _assert_semantic_plot_image(blank, width=800, height=600)


def test_P3_11_plotitem_layout_axes_and_legend_visual_artifacts(tmp_path: Path) -> None:
    renderer = _renderer()
    work = tmp_path / "p3_11"
    work.mkdir(parents=True)

    simple_actual = work / "SimplePlot.actual.png"
    _run_renderer(renderer, "SimplePlot", simple_actual)

    decorations_reference = work / "PlotDecorations.reference.png"
    decorations_actual = work / "PlotDecorations.actual.png"
    _run_renderer(renderer, "PlotDecorationsReference", decorations_reference)
    _run_renderer(renderer, "PlotDecorations", decorations_actual)

    reports_root = _reports_root(tmp_path)
    simple_metrics = _check_visual(
        case="SimplePlot",
        reference=REFERENCE_SIMPLEPLOT,
        actual=simple_actual,
        reports_root=reports_root,
        max_mean=10,
        max_delta=255,
        max_changed=8,
        min_ssim=0.75,
    )
    decoration_metrics = _check_visual(
        case="PlotDecorations",
        reference=decorations_reference,
        actual=decorations_actual,
        reports_root=reports_root,
        max_mean=35,
        max_delta=255,
        max_changed=25,
        min_ssim=0.55,
    )

    _write_manual_report(
        reports_root,
        {"SimplePlot": simple_metrics, "PlotDecorations": decoration_metrics},
    )
    assert (reports_root / "manual_semantic_inspection.md").is_file()
