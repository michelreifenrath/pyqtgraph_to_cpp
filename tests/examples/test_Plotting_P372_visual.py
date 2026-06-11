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

REFERENCE = ROOT / "oracle" / "fixtures" / "screenshots" / "Plotting.reference.png"
CHECK_VISUAL_ARTIFACTS = ROOT / "scripts" / "check_visual_artifacts"
DEFAULT_REVIEW = ROOT / "reports" / "examples" / "P3.72" / "gpt5_vision_review.md"
PYQTGRAPH_REF = (
    "/home/michel/.cache/pgcpp-opensrc/repos/github.com/pyqtgraph/pyqtgraph/"
    "pyqtgraph-0.14.0"
)


def _renderer() -> Path:
    renderer_env = os.environ.get("PG_CPP_P372_VISUAL_RENDERER")
    if not renderer_env:
        pytest.skip(
            "PG_CPP_P372_VISUAL_RENDERER is provided by CTest; run "
            "`ctest --preset visual -L P3.72 --output-on-failure`"
        )
    renderer = Path(renderer_env)
    assert renderer.is_file(), f"P3.72 visual renderer does not exist: {renderer}"
    return renderer


def _reports_root(tmp_path: Path) -> Path:
    configured = os.environ.get("PG_P372_VISUAL_REPORTS_ROOT")
    return Path(configured) if configured else tmp_path / "reports" / "visual-diffs"


def _example_reports_root(tmp_path: Path) -> Path:
    configured = os.environ.get("PG_P372_EXAMPLES_REPORTS_ROOT")
    return Path(configured) if configured else tmp_path / "reports" / "examples" / "P3.72"


def _review_source(tmp_path: Path) -> Path:
    configured = Path(os.environ.get("PG_P372_VISUAL_REVIEW_REPORT", str(DEFAULT_REVIEW)))
    assert configured.is_file(), f"missing P3.72 GPT visual review source: {configured}"
    review_copy = tmp_path / "gpt5_vision_review.md"
    shutil.copyfile(configured, review_copy)
    return review_copy


def _run_renderer(renderer: Path, output: Path) -> dict[str, Any]:
    result = subprocess.run(
        [
            str(renderer),
            "Plotting",
            "--output",
            str(output),
            "--width",
            "1000",
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
    assert status["example"] == "Plotting"
    assert status["dimensions"] == {"width": 1000, "height": 600}
    assert status["placeholder"] is False
    assert status["render_path"] == "QWidget::grab"
    assert Path(str(status["output"])).resolve() == output.resolve()
    assert output.is_file()
    _assert_semantic_plot_image(output, width=1000, height=600)
    return status


def _check_visual_artifacts(
    *, actual: Path, reports_root: Path, review_source: Path
) -> dict[str, Any]:
    result = subprocess.run(
        [
            sys.executable,
            str(CHECK_VISUAL_ARTIFACTS),
            "--case",
            "Plotting",
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
            "8",
            "--max-pixel-delta",
            "240",
            "--max-changed-percent",
            "8",
            "--min-ssim",
            "0.75",
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr + result.stdout
    metrics_path = reports_root / "Plotting" / "metrics.json"
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
        assert (reports_root / "Plotting" / artifact_name).is_file()
    return metrics


def _write_example_report(example_root: Path, metrics: dict[str, Any]) -> None:
    example_root.mkdir(parents=True, exist_ok=True)
    artifact_paths = dict(metrics["artifact_paths"])
    artifact_paths["gpt5_vision_review"] = str(metrics["review_report_path"])
    report = {
        "issue": "P3.72",
        "owned_examples": [
            {
                "path": "examples/Plotting.cpp",
                "upstream_reference": "pyqtgraph/examples/Plotting.py",
                "validation_level": {
                    "numeric": "required",
                    "visual": "required",
                    "interaction": "required",
                    "gpt_visual_review": "required_for_pr",
                },
                "artifact_paths": artifact_paths,
                "pass": True,
                "not_applicable": {},
                "manual_agent_image_inspection": (
                    "Reference and C++ actual images show the same 1000x600 "
                    "3x3 GraphicsLayoutWidget with nine titled plot panels, grid/log/"
                    "scatter/fill/region/zoom content, and no blank or placeholder panes."
                ),
            }
        ],
        "pyqtgraph_reference": {
            "ref": "pyqtgraph-0.14.0",
            "pinned_commit": "a20028b98294b9cc8770f2015a92eb342224b788",
            "files": [
                "pyqtgraph/examples/Plotting.py",
                "pyqtgraph/widgets/GraphicsLayoutWidget.py",
                "pyqtgraph/graphicsItems/PlotItem/PlotItem.py",
                "pyqtgraph/graphicsItems/LinearRegionItem.py",
            ],
            "path": PYQTGRAPH_REF,
        },
    }
    (example_root / "report.json").write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    (example_root / "README.md").write_text(
        "# P3.72 Plotting example-port report\n\n"
        "- Example: `examples/Plotting.cpp`\n"
        "- Validation: numeric, visual, and interaction required.\n"
        f"- Visual metrics: `{artifact_paths['metrics']}`\n"
        f"- Reference image: `{artifact_paths['reference']}`\n"
        f"- Actual image: `{artifact_paths['actual']}`\n"
        f"- Diff image: `{artifact_paths['diff']}`\n"
        f"- GPT visual review: `{artifact_paths['gpt5_vision_review']}`\n"
        "- Status: pass.\n",
        encoding="utf-8",
    )


def test_P372_plotting_builds_renders_and_writes_example_report(tmp_path: Path) -> None:
    renderer = _renderer()
    actual = tmp_path / "Plotting.actual.png"
    _run_renderer(renderer, actual)

    metrics = _check_visual_artifacts(
        actual=actual,
        reports_root=_reports_root(tmp_path),
        review_source=_review_source(tmp_path),
    )
    _write_example_report(_example_reports_root(tmp_path), metrics)

    example_root = _example_reports_root(tmp_path)
    report = json.loads((example_root / "report.json").read_text(encoding="utf-8"))
    assert report["issue"] == "P3.72"
    assert report["owned_examples"][0]["path"] == "examples/Plotting.cpp"
    assert report["owned_examples"][0]["validation_level"]["visual"] == "required"
    assert report["owned_examples"][0]["validation_level"]["interaction"] == "required"
    assert report["owned_examples"][0]["pass"] is True
    assert (example_root / "README.md").is_file()
