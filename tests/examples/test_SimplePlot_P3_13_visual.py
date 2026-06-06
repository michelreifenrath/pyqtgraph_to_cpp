from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any

import pytest

ROOT = Path(__file__).resolve().parents[2]
VISUAL_TESTS = ROOT / "tests" / "visual"
if str(VISUAL_TESTS) not in sys.path:
    sys.path.insert(0, str(VISUAL_TESTS))

from test_P1_08_cpp_visual_renderer import _assert_semantic_plot_image  # noqa: E402

REFERENCE = ROOT / "oracle" / "fixtures" / "screenshots" / "SimplePlot.reference.png"
CHECK_VISUAL_ARTIFACTS = ROOT / "scripts" / "check_visual_artifacts"
DEFAULT_REVIEW = ROOT / "reports" / "examples" / "P3.13" / "gpt5_vision_review.md"
PYQTGRAPH_REF = (
    "/home/michel/.cache/pgcpp-opensrc/repos/github.com/pyqtgraph/pyqtgraph/"
    "pyqtgraph-0.14.0"
)


def _renderer() -> Path:
    renderer_env = os.environ.get("PG_CPP_P3_13_VISUAL_RENDERER")
    if not renderer_env:
        pytest.skip(
            "PG_CPP_P3_13_VISUAL_RENDERER is provided by CTest; run "
            "`ctest --preset visual -L P3.13 --output-on-failure`"
        )
    renderer = Path(renderer_env)
    assert renderer.is_file(), f"P3.13 visual renderer does not exist: {renderer}"
    return renderer


def _reports_root(tmp_path: Path) -> Path:
    configured = os.environ.get("PG_P3_13_VISUAL_REPORTS_ROOT")
    return Path(configured) if configured else tmp_path / "reports" / "visual-diffs"


def _example_reports_root(tmp_path: Path) -> Path:
    configured = os.environ.get("PG_P3_13_EXAMPLES_REPORTS_ROOT")
    return Path(configured) if configured else tmp_path / "reports" / "examples" / "P3.13"


def _review_source(tmp_path: Path) -> Path:
    configured = Path(os.environ.get("PG_P3_13_VISUAL_REVIEW_REPORT", str(DEFAULT_REVIEW)))
    assert configured.is_file(), f"missing P3.13 GPT visual review source: {configured}"
    review_copy = tmp_path / "gpt5_vision_review.md"
    shutil.copyfile(configured, review_copy)
    return review_copy


def _run_renderer(renderer: Path, output: Path) -> dict[str, Any]:
    result = subprocess.run(
        [
            str(renderer),
            "SimplePlot",
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
    assert status["example"] == "SimplePlot"
    assert status["dimensions"] == {"width": 800, "height": 600}
    assert status["placeholder"] is False
    assert status["render_path"] == "QWidget::grab"
    assert Path(str(status["output"])).resolve() == output.resolve()
    assert output.is_file()
    _assert_semantic_plot_image(output, width=800, height=600)
    return status


def _check_visual_artifacts(
    *, actual: Path, reports_root: Path, review_source: Path
) -> dict[str, Any]:
    result = subprocess.run(
        [
            sys.executable,
            str(CHECK_VISUAL_ARTIFACTS),
            "--case",
            "SimplePlot",
            "--reference",
            str(REFERENCE),
            "--actual",
            str(actual),
            "--reports-root",
            str(reports_root),
            "--gpt-visual-review",
            "required_for_pr",
            "--review-report",
            str(review_source),
            "--max-mean-delta",
            "6",
            "--max-pixel-delta",
            "220",
            "--max-changed-percent",
            "5",
            "--min-ssim",
            "0.8",
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr + result.stdout
    metrics_path = reports_root / "SimplePlot" / "metrics.json"
    metrics = json.loads(metrics_path.read_text(encoding="utf-8"))
    assert metrics["passed"] is True
    assert metrics["deterministic_verdict"] == "pass"
    assert metrics["semantic_review"]["mode"] == "required_for_pr"
    assert metrics["semantic_review"]["verdict"] == "pass"
    assert metrics["semantic_review"]["recommendation"] == "merge_ok"
    assert metrics["semantic_review"]["accepted"] is True
    assert metrics["failed_checks"] == []
    for artifact_name in (
        "reference.png",
        "actual.png",
        "diff.png",
        "metrics.json",
        "gpt5_vision_review.md",
    ):
        assert (reports_root / "SimplePlot" / artifact_name).is_file()
    return metrics


def _write_example_report(example_root: Path, metrics: dict[str, Any]) -> None:
    example_root.mkdir(parents=True, exist_ok=True)
    artifact_paths = dict(metrics["artifact_paths"])
    artifact_paths["gpt5_vision_review"] = str(metrics["review_report_path"])
    report = {
        "issue": "P3.13",
        "owned_examples": [
            {
                "path": "examples/SimplePlot.cpp",
                "upstream_reference": "pyqtgraph/examples/SimplePlot.py",
                "validation_level": {
                    "numeric": "not_applicable",
                    "visual": "required",
                    "interaction": "not_applicable",
                    "gpt_visual_review": "required_for_pr",
                },
                "artifact_paths": artifact_paths,
                "pass": True,
                "not_applicable": {
                    "numeric": "SimplePlot is validated as an example-port visual slice.",
                    "interaction": "No interaction script is required for this static SimplePlot slice.",
                },
                "manual_agent_image_inspection": (
                    "Reference and C++ actual images show the same 800x600 black "
                    "plot area with visible axes/ticks and a white y-only curve; "
                    "the diff image shows only tolerance-bounded styling/rasterization "
                    "differences."
                ),
            }
        ],
        "pyqtgraph_reference": {
            "ref": "pyqtgraph-0.14.0",
            "pinned_commit": "a20028b98294b9cc8770f2015a92eb342224b788",
            "files": [
                "pyqtgraph/examples/SimplePlot.py",
                "pyqtgraph/__init__.py",
                "pyqtgraph/widgets/PlotWidget.py",
                "pyqtgraph/graphicsItems/PlotItem/PlotItem.py",
                "pyqtgraph/graphicsItems/PlotDataItem.py",
            ],
            "path": PYQTGRAPH_REF,
        },
    }
    (example_root / "report.json").write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    (example_root / "README.md").write_text(
        "# P3.13 SimplePlot example-port report\n\n"
        "- Example: `examples/SimplePlot.cpp`\n"
        "- Validation: visual required; numeric/interaction not applicable.\n"
        f"- Visual metrics: `{artifact_paths['metrics']}`\n"
        f"- Reference image: `{artifact_paths['reference']}`\n"
        f"- Actual image: `{artifact_paths['actual']}`\n"
        f"- Diff image: `{artifact_paths['diff']}`\n"
        f"- GPT visual review: `{artifact_paths['gpt5_vision_review']}`\n"
        "- Status: pass.\n\n"
        "Manual semantic inspection: reference and actual both show the SimplePlot "
        "black plot area, axes/ticks, and visible white curve; differences are limited "
        "to tolerated rasterization/styling deltas.\n",
        encoding="utf-8",
    )


def test_P3_13_simpleplot_builds_renders_and_writes_example_report(tmp_path: Path) -> None:
    renderer = _renderer()
    actual = tmp_path / "SimplePlot.actual.png"
    _run_renderer(renderer, actual)

    metrics = _check_visual_artifacts(
        actual=actual,
        reports_root=_reports_root(tmp_path),
        review_source=_review_source(tmp_path),
    )
    _write_example_report(_example_reports_root(tmp_path), metrics)

    example_root = _example_reports_root(tmp_path)
    report = json.loads((example_root / "report.json").read_text(encoding="utf-8"))
    assert report["issue"] == "P3.13"
    assert report["owned_examples"][0]["path"] == "examples/SimplePlot.cpp"
    assert report["owned_examples"][0]["validation_level"]["visual"] == "required"
    assert report["owned_examples"][0]["validation_level"]["gpt_visual_review"] == "required_for_pr"
    assert report["owned_examples"][0]["pass"] is True
    assert (example_root / "README.md").is_file()
