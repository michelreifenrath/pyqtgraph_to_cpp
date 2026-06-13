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

from plotting_subplot_visual import (  # noqa: E402
    WHOLE_IMAGE_TOLERANCE,
    assert_all_subplots_nonempty,
    assert_plotting_auto_button_layout,
    blank_subplot,
    compare_render_pair_subplots,
    compare_subplots,
    crop_subplot_rgba,
    degenerate_axis_subplots,
    write_subplot_png,
)
from test_P1_08_cpp_visual_renderer import _assert_semantic_plot_image  # noqa: E402
from compare_screenshots import read_png_rgba  # noqa: E402

REFERENCE = ROOT / "oracle" / "fixtures" / "screenshots" / "Plotting.reference.png"
PLOTTING_DATA_FIXTURE = ROOT / "oracle" / "fixtures" / "P372" / "plotting_data.json"
P8_REFERENCE = ROOT / "oracle" / "fixtures" / "P372" / "screenshots" / "Plotting.p8.reference.png"
P3_REFERENCE = ROOT / "oracle" / "fixtures" / "P372" / "screenshots" / "Plotting.p3.reference.png"
P5_REFERENCE = ROOT / "oracle" / "fixtures" / "P372" / "screenshots" / "Plotting.p5.reference.png"
P4_REFERENCE = ROOT / "oracle" / "fixtures" / "P372" / "screenshots" / "Plotting.p4.reference.png"
CHECK_VISUAL_ARTIFACTS = ROOT / "scripts" / "check_visual_artifacts"
DEFAULT_REVIEW = ROOT / "reports" / "examples" / "P3.72" / "gpt5_vision_review.md"
PYQTGRAPH_REF = ROOT / "reference" / "pyqtgraph"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"

# Measured from pinned PyQtGraph p3 crop (Plotting.p3.reference.png).
# C++ default symbolSize is 7 vs PyQtGraph 10, so red area is ~47% of reference.
P3_MIN_RED_MARKER_PIXELS = 2000
P3_MIN_WHITE_RIM_PIXELS = 1200
P3_RED_COVERAGE_RATIO = 0.40
P3_WHITE_COVERAGE_RATIO = 0.80

# Measured from pinned PyQtGraph p5 crop (Plotting.p5.reference.png).
P5_MIN_BLUE_TRIANGLE_PIXELS = 3000
P5_TRIANGLE_COVERAGE_RATIO = 0.85

# Measured from pinned PyQtGraph p4 crop (Plotting.p4.reference.png).
P4_MIN_GRAY_GRID_PIXELS = 900
P4_GRID_COVERAGE_RATIO = 0.75
P4_MIN_CURVE_PIXELS = 30000
P4_MIN_HORIZONTAL_GRID_LINES = 3
P4_MIN_VERTICAL_GRID_LINES = 8

# p1 never calls showGrid; grid-like pixels should stay sparse.
P1_MAX_GRAY_GRID_PIXELS = 250

# Measured from pinned PyQtGraph p8 crop (Plotting.p8.reference.png).
P8_MIN_EDGE_LINE_PIXELS = 200
P8_MIN_BAND_COVERAGE_RATIO = 0.85


def _count_p3_region_pixels(rgba: bytes) -> tuple[int, int]:
    red_markers = 0
    white_rim = 0
    for index in range(0, len(rgba), 4):
        red, green, blue, alpha = rgba[index : index + 4]
        if alpha > 100 and red > 200 and green < 80 and blue < 80:
            red_markers += 1
        if alpha > 100 and red > 200 and green > 200 and blue > 200:
            white_rim += 1
    return red_markers, white_rim


def _count_p5_region_pixels(rgba: bytes) -> int:
    blue_triangles = 0
    for index in range(0, len(rgba), 4):
        red, green, blue, alpha = rgba[index : index + 4]
        if alpha > 20 and blue > 100 and red < 120 and green < 120:
            blue_triangles += 1
    return blue_triangles


def _assert_p3_region_markers(actual: Path) -> None:
    assert P3_REFERENCE.is_file(), f"missing pinned p3 reference crop: {P3_REFERENCE}"
    _, _, ref_rgba = read_png_rgba(P3_REFERENCE)
    ref_red, ref_white = _count_p3_region_pixels(ref_rgba)
    assert ref_red >= P3_MIN_RED_MARKER_PIXELS
    assert ref_white >= P3_MIN_WHITE_RIM_PIXELS

    width, height, rgba = read_png_rgba(actual)
    _, _, crop_rgba = crop_subplot_rgba(rgba, width, height, col=2, row=0)
    red_pixels, white_pixels = _count_p3_region_pixels(crop_rgba)
    min_red = max(P3_MIN_RED_MARKER_PIXELS, int(P3_RED_COVERAGE_RATIO * ref_red))
    min_white = max(P3_MIN_WHITE_RIM_PIXELS, int(P3_WHITE_COVERAGE_RATIO * ref_white))
    assert red_pixels >= min_red, (
        f"p3 red marker coverage too low: {red_pixels} < {min_red} (reference={ref_red})"
    )
    assert white_pixels >= min_white, (
        f"p3 white rim coverage too low: {white_pixels} < {min_white} (reference={ref_white})"
    )
    assert red_pixels > white_pixels, (
        f"p3 red interior should dominate white rim: {red_pixels} <= {white_pixels}"
    )

    tmp_crop = actual.parent / "Plotting.p3.actual.png"
    write_subplot_png(actual, tmp_crop, col=2, row=0)


def _assert_p5_region_triangles(actual: Path) -> None:
    assert P5_REFERENCE.is_file(), f"missing pinned p5 reference crop: {P5_REFERENCE}"
    _, _, ref_rgba = read_png_rgba(P5_REFERENCE)
    ref_blue = _count_p5_region_pixels(ref_rgba)
    assert ref_blue >= P5_MIN_BLUE_TRIANGLE_PIXELS

    width, height, rgba = read_png_rgba(actual)
    _, _, crop_rgba = crop_subplot_rgba(rgba, width, height, col=1, row=1)
    blue_pixels = _count_p5_region_pixels(crop_rgba)
    min_blue = int(P5_TRIANGLE_COVERAGE_RATIO * ref_blue)
    assert blue_pixels >= min_blue, (
        f"p5 blue triangle coverage too low: {blue_pixels} < {min_blue} (reference={ref_blue})"
    )

    tmp_crop = actual.parent / "Plotting.p5.actual.png"
    write_subplot_png(actual, tmp_crop, col=1, row=1)


def _is_gray_grid_pixel(red: int, green: int, blue: int, alpha: int) -> bool:
    return (
        alpha >= 25
        and 90 <= red <= 190
        and 90 <= green <= 190
        and 90 <= blue <= 190
        and max(abs(red - green), abs(green - blue)) < 35
    )


def _count_p4_region_pixels(rgba: bytes, width: int, height: int) -> tuple[int, int, int, int]:
    gray_grid = 0
    curve_pixels = 0
    horizontal_lines: set[int] = set()
    vertical_lines: set[int] = set()
    for y in range(height):
        horizontal_run = 0
        for x in range(width):
            index = (y * width + x) * 4
            red, green, blue, alpha = rgba[index : index + 4]
            if _is_gray_grid_pixel(red, green, blue, alpha):
                gray_grid += 1
                horizontal_run += 1
                if y > 0:
                    above = rgba[((y - 1) * width + x) * 4 : ((y - 1) * width + x) * 4 + 4]
                    if _is_gray_grid_pixel(*above):
                        horizontal_lines.add(y)
                if x > 0:
                    left = rgba[(y * width + (x - 1)) * 4 : (y * width + (x - 1)) * 4 + 4]
                    if _is_gray_grid_pixel(*left):
                        vertical_lines.add(x)
            else:
                if horizontal_run >= 8:
                    horizontal_lines.add(y)
                horizontal_run = 0
            if alpha >= 40 and red < 40 and green < 40 and blue < 40:
                curve_pixels += 1
        if horizontal_run >= 8:
            horizontal_lines.add(y)
    return gray_grid, curve_pixels, len(horizontal_lines), len(vertical_lines)


def _assert_p4_grid_region(actual: Path) -> None:
    assert P4_REFERENCE.is_file(), f"missing pinned p4 reference crop: {P4_REFERENCE}"
    ref_w, ref_h, ref_rgba = read_png_rgba(P4_REFERENCE)
    ref_gray, ref_curve, ref_h_lines, ref_v_lines = _count_p4_region_pixels(
        ref_rgba, ref_w, ref_h
    )
    assert ref_gray >= P4_MIN_GRAY_GRID_PIXELS
    assert ref_h_lines >= P4_MIN_HORIZONTAL_GRID_LINES
    assert ref_v_lines >= P4_MIN_VERTICAL_GRID_LINES
    assert ref_curve >= P4_MIN_CURVE_PIXELS

    width, height, rgba = read_png_rgba(actual)
    crop_width, crop_height, crop_rgba = crop_subplot_rgba(rgba, width, height, col=0, row=1)
    gray_pixels, curve_pixels, h_lines, v_lines = _count_p4_region_pixels(
        crop_rgba, crop_width, crop_height
    )
    min_gray = max(P4_MIN_GRAY_GRID_PIXELS, int(P4_GRID_COVERAGE_RATIO * ref_gray))
    assert gray_pixels >= min_gray, (
        f"p4 gray grid coverage too low: {gray_pixels} < {min_gray} (reference={ref_gray})"
    )
    assert h_lines >= P4_MIN_HORIZONTAL_GRID_LINES, (
        f"p4 horizontal grid lines too sparse: {h_lines} < {P4_MIN_HORIZONTAL_GRID_LINES}"
    )
    assert v_lines >= P4_MIN_VERTICAL_GRID_LINES, (
        f"p4 vertical grid lines too sparse: {v_lines} < {P4_MIN_VERTICAL_GRID_LINES}"
    )
    assert curve_pixels >= int(0.8 * ref_curve), (
        f"p4 curve should render above grid: {curve_pixels} < {int(0.8 * ref_curve)}"
    )
    assert curve_pixels > gray_pixels, (
        f"p4 curve pixels should dominate grid pixels: {curve_pixels} <= {gray_pixels}"
    )

    tmp_crop = actual.parent / "Plotting.p4.actual.png"
    write_subplot_png(actual, tmp_crop, col=0, row=1)


def _assert_p1_no_grid_region(actual: Path) -> None:
    width, height, rgba = read_png_rgba(actual)
    crop_width, crop_height, crop_rgba = crop_subplot_rgba(rgba, width, height, col=0, row=0)
    gray_pixels, _, h_lines, v_lines = _count_p4_region_pixels(
        crop_rgba, crop_width, crop_height
    )
    assert gray_pixels <= P1_MAX_GRAY_GRID_PIXELS, (
        f"p1 should not show grid pixels: {gray_pixels} > {P1_MAX_GRAY_GRID_PIXELS}"
    )
    assert h_lines <= 5 and v_lines <= 5, (
        f"p1 should not show grid line structure: h={h_lines} v={v_lines}"
    )

    tmp_crop = actual.parent / "Plotting.p1.grid_probe.actual.png"
    write_subplot_png(actual, tmp_crop, col=0, row=0)


def _count_p8_region_pixels(rgba: bytes) -> tuple[int, int, int]:
    translucent_band = 0
    edge_lines = 0
    relaxed_band = 0
    for index in range(0, len(rgba), 4):
        red, green, blue, alpha = rgba[index : index + 4]
        if blue > 100 and red < 80 and green < 80 and alpha > 20:
            translucent_band += 1
        if blue > red + 15 and blue > green + 15 and alpha > 100:
            relaxed_band += 1
        if red > 150 and green > 150 and blue < 120 and alpha > 100:
            edge_lines += 1
    return translucent_band, edge_lines, relaxed_band


def _assert_p8_region_band(actual: Path) -> None:
    assert P8_REFERENCE.is_file(), f"missing pinned p8 reference crop: {P8_REFERENCE}"
    _, _, ref_rgba = read_png_rgba(P8_REFERENCE)
    ref_band, ref_edges, ref_relaxed = _count_p8_region_pixels(ref_rgba)
    assert ref_edges >= P8_MIN_EDGE_LINE_PIXELS
    assert ref_relaxed > 0

    width, height, rgba = read_png_rgba(actual)
    _, _, crop_rgba = crop_subplot_rgba(rgba, width, height, col=1, row=2)
    _, edge_pixels, relaxed_band = _count_p8_region_pixels(crop_rgba)
    min_relaxed = int(P8_MIN_BAND_COVERAGE_RATIO * ref_relaxed)
    assert relaxed_band >= min_relaxed, (
        f"p8 translucent band coverage too low: {relaxed_band} < {min_relaxed} "
        f"(reference={ref_relaxed}, strict_band={ref_band})"
    )
    assert edge_pixels >= P8_MIN_EDGE_LINE_PIXELS, (
        f"p8 edge lines too sparse: {edge_pixels} < {P8_MIN_EDGE_LINE_PIXELS} "
        f"(reference={ref_edges})"
    )

    tmp_crop = actual.parent / "Plotting.p8.actual.png"
    write_subplot_png(actual, tmp_crop, col=1, row=2)


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


def _render_plotting(
    renderer: Path,
    output: Path,
    *,
    extra_args: list[str] | None = None,
) -> dict[str, Any]:
    command = [
        str(renderer),
        "Plotting",
        "--output",
        str(output),
        "--width",
        "1000",
        "--height",
        "600",
        "--data-fixture",
        str(PLOTTING_DATA_FIXTURE),
    ]
    if extra_args:
        command.extend(extra_args)
    result = subprocess.run(
        command,
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
    return status


def _run_renderer(
    renderer: Path,
    output: Path,
    *,
    extra_args: list[str] | None = None,
) -> dict[str, Any]:
    status = _render_plotting(renderer, output, extra_args=extra_args)
    _assert_semantic_plot_image(output, width=1000, height=600)
    _assert_p3_region_markers(output)
    _assert_p4_grid_region(output)
    _assert_p1_no_grid_region(output)
    _assert_p5_region_triangles(output)
    _assert_p8_region_band(output)
    assert_plotting_auto_button_layout(output)
    assert_all_subplots_nonempty(output)
    return status


def _check_visual_artifacts(
    *, actual: Path, reports_root: Path, review_source: Path
) -> dict[str, Any]:
    tolerance = WHOLE_IMAGE_TOLERANCE
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
            str(tolerance["max_mean_delta"]),
            "--max-pixel-delta",
            str(tolerance["max_pixel_delta"]),
            "--max-changed-percent",
            str(tolerance["max_changed_percent"]),
            "--min-ssim",
            str(tolerance["min_ssim"]),
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

    subplot_metrics = compare_subplots(
        REFERENCE,
        actual,
        reports_dir=reports_root / "Plotting" / "subplots",
    )
    assert subplot_metrics["passed"] is True, subplot_metrics["failed_cells"]
    metrics["subplot_metrics"] = subplot_metrics

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
                    "Reference comes from pinned PyQtGraph rendering with seeded data "
                    "arrays exported from reference/pyqtgraph/examples/Plotting.py "
                    f"(np.random.seed(0x504C5454)). C++ visual tests load the same "
                    "fixture via --data-fixture. Whole-image thresholds stay loose; "
                    "tightening is per-subplot pixel gates for p1-p5/p7-p9 plus "
                    "p4/p8 semantic min-coverage checks and p6 non-emptiness."
                ),
            }
        ],
        "pyqtgraph_reference": {
            "ref": "pyqtgraph-0.14.0",
            "pinned_commit": PINNED_COMMIT,
            "files": [
                "pyqtgraph/examples/Plotting.py",
                "pyqtgraph/widgets/GraphicsLayoutWidget.py",
                "pyqtgraph/graphicsItems/PlotItem/PlotItem.py",
                "pyqtgraph/graphicsItems/LinearRegionItem.py",
            ],
            "path": str(PYQTGRAPH_REF),
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


def _assert_visual_gate_fails(actual: Path, tmp_path: Path) -> None:
    tolerance = WHOLE_IMAGE_TOLERANCE
    reports_root = tmp_path / "negative-reports"
    result = subprocess.run(
        [
            sys.executable,
            str(CHECK_VISUAL_ARTIFACTS),
            "--case",
            "Plotting-negative",
            "--reference",
            str(REFERENCE),
            "--actual",
            str(actual),
            "--reports-root",
            str(reports_root),
            "--gpt-visual-review",
            "not_applicable",
            "--max-mean-delta",
            str(tolerance["max_mean_delta"]),
            "--max-pixel-delta",
            str(tolerance["max_pixel_delta"]),
            "--max-changed-percent",
            str(tolerance["max_changed_percent"]),
            "--min-ssim",
            str(tolerance["min_ssim"]),
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    whole_failed = result.returncode != 0
    subplot_metrics = compare_subplots(REFERENCE, actual)
    nonempty_failed = False
    try:
        assert_all_subplots_nonempty(actual)
    except AssertionError:
        nonempty_failed = True
    assert whole_failed or not subplot_metrics["passed"] or nonempty_failed


def _assert_subplot_gate_fails(actual: Path, expected_cells: set[str]) -> None:
    subplot_metrics = compare_subplots(REFERENCE, actual)
    assert not subplot_metrics["passed"], subplot_metrics
    assert expected_cells.issubset(set(subplot_metrics["failed_cells"])), (
        f"expected failures in {sorted(expected_cells)}, "
        f"got {subplot_metrics['failed_cells']}"
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


def test_P372_plotting_consecutive_fixture_renders_are_deterministic(tmp_path: Path) -> None:
    renderer = _renderer()
    first = tmp_path / "Plotting.first.png"
    second = tmp_path / "Plotting.second.png"
    _render_plotting(renderer, first)
    _render_plotting(renderer, second)

    determinism_metrics = compare_render_pair_subplots(first, second)
    assert determinism_metrics["passed"] is True, determinism_metrics["failed_cells"]


def test_P372_plotting_wrong_symbol_fixture_fails_p3_cell(tmp_path: Path) -> None:
    renderer = _renderer()
    actual = tmp_path / "Plotting.wrong_symbol.png"
    _render_plotting(renderer, actual, extra_args=["--plotting-wrong-symbol"])
    _assert_subplot_gate_fails(actual, {"p3"})


def test_P372_plotting_disabled_grid_fixture_fails_p4_cell(tmp_path: Path) -> None:
    renderer = _renderer()
    actual = tmp_path / "Plotting.no_grid.png"
    _render_plotting(renderer, actual, extra_args=["--plotting-no-grid"])
    _assert_subplot_gate_fails(actual, {"p4"})


def test_P372_plotting_hidden_region_fixture_fails_p8_cell(tmp_path: Path) -> None:
    renderer = _renderer()
    actual = tmp_path / "Plotting.hide_region.png"
    _render_plotting(renderer, actual, extra_args=["--plotting-hide-region"])
    _assert_subplot_gate_fails(actual, {"p8"})


def test_P372_plotting_blank_p5_fixture_fails_visual_gate(tmp_path: Path) -> None:
    renderer = _renderer()
    actual = tmp_path / "Plotting.actual.png"
    _run_renderer(renderer, actual)

    blanked = tmp_path / "Plotting.blank_p5.png"
    blank_subplot(actual, blanked, col=1, row=1)
    _assert_visual_gate_fails(blanked, tmp_path)


def test_P372_plotting_runaway_axis_fixture_fails_visual_gate(tmp_path: Path) -> None:
    renderer = _renderer()
    actual = tmp_path / "Plotting.actual.png"
    _run_renderer(renderer, actual)

    runaway = tmp_path / "Plotting.runaway_axis.png"
    degenerate_axis_subplots(actual, runaway, reference=REFERENCE)

    subplot_metrics = compare_subplots(REFERENCE, runaway)
    assert "p8" in subplot_metrics["failed_cells"]
    assert "p9" in subplot_metrics["failed_cells"]
    assert "p5" not in subplot_metrics["failed_cells"]
    assert_all_subplots_nonempty(runaway)
    _assert_visual_gate_fails(runaway, tmp_path)
