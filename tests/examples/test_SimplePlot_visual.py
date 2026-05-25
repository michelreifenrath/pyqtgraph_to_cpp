from __future__ import annotations

import importlib.util
import json
import shutil
import struct
import subprocess
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
REFERENCE = ROOT / "oracle" / "fixtures" / "screenshots" / "SimplePlot.reference.png"
INTERACTION = ROOT / "oracle" / "fixtures" / "interactions" / "SimplePlot.json"
CPP_RENDERER = ROOT / "oracle" / "scripts" / "render_cpp_example.py"
COMPARATOR = ROOT / "oracle" / "scripts" / "compare_screenshots.py"
INTERACTION_RUNNER = ROOT / "oracle" / "scripts" / "run_interaction_script.py"
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def run_cli(script: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(script), *args],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )


def import_interaction_runner() -> Any:
    spec = importlib.util.spec_from_file_location("run_interaction_script", INTERACTION_RUNNER)
    assert spec is not None
    assert spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def read_png_dimensions(path: Path) -> tuple[int, int]:
    data = path.read_bytes()
    assert data.startswith(PNG_SIGNATURE)
    length = struct.unpack(">I", data[len(PNG_SIGNATURE) : len(PNG_SIGNATURE) + 4])[0]
    kind = data[len(PNG_SIGNATURE) + 4 : len(PNG_SIGNATURE) + 8]
    assert length == 13
    assert kind == b"IHDR"
    width, height, bit_depth, color_type, compression, filter_method, interlace = (
        struct.unpack(
            ">IIBBBBB", data[len(PNG_SIGNATURE) + 8 : len(PNG_SIGNATURE) + 21]
        )
    )
    assert bit_depth == 8
    assert color_type in (2, 6)
    assert compression == 0
    assert filter_method == 0
    assert interlace == 0
    return width, height


def run_compare(
    reference: Path, candidate: Path, diff: Path, metrics: Path
) -> subprocess.CompletedProcess[str]:
    return run_cli(
        COMPARATOR,
        str(reference),
        str(candidate),
        "--diff",
        str(diff),
        "--metrics",
        str(metrics),
    )


def test_reference_screenshot_fixture_is_800x600_png() -> None:
    assert REFERENCE.is_file()
    assert read_png_dimensions(REFERENCE) == (800, 600)


def test_simpleplot_interaction_fixture_loads_as_empty_yaml_compatible_json() -> None:
    assert INTERACTION.is_file()
    runner = import_interaction_runner()

    assert json.loads(INTERACTION.read_text(encoding="utf-8")) == {
        "version": 1,
        "steps": [],
    }
    assert runner.load_interaction_script(INTERACTION) == []


def test_simpleplot_visual_oracle_generates_placeholder_diff_artifacts(
    tmp_path: Path,
) -> None:
    artifact_dir = tmp_path / "reports" / "visual-diffs" / "SimplePlot"
    reference = artifact_dir / "reference.png"
    actual = artifact_dir / "actual.png"
    diff = artifact_dir / "diff.png"
    metrics_path = artifact_dir / "metrics.json"
    artifact_dir.mkdir(parents=True)
    shutil.copyfile(REFERENCE, reference)

    render_result = run_cli(
        CPP_RENDERER,
        "SimplePlot",
        "--output",
        str(actual),
        "--width",
        "800",
        "--height",
        "600",
    )

    assert render_result.returncode == 0, render_result.stderr
    assert render_result.stderr == ""
    render_status = json.loads(render_result.stdout)
    assert render_status["example"] == "SimplePlot"
    assert render_status["dimensions"] == {"width": 800, "height": 600}
    assert render_status["placeholder"] is True
    assert read_png_dimensions(actual) == (800, 600)

    compare_result = run_compare(reference, actual, diff, metrics_path)

    assert compare_result.returncode == 1, compare_result.stderr
    assert compare_result.stderr == ""
    assert diff.is_file()
    assert metrics_path.is_file()
    metrics = json.loads(metrics_path.read_text(encoding="utf-8"))
    assert json.loads(compare_result.stdout) == metrics
    assert metrics["passed"] is False
    assert metrics["deterministic_verdict"] == "fail"
    assert metrics["dimensions"] == {
        "reference": {"width": 800, "height": 600},
        "candidate": {"width": 800, "height": 600},
    }
    assert metrics["reference_path"] == str(reference)
    assert metrics["candidate_path"] == str(actual)
    assert metrics["diff_image_path"] == str(diff)
    assert metrics["changed_pixel_percentage"] > 0
    assert metrics["mean_absolute_delta"] > 0
    assert metrics["max_delta"] > 0
    assert metrics["failed_tolerances"]


def test_simpleplot_reference_compares_identically_to_itself(tmp_path: Path) -> None:
    diff = tmp_path / "diff.png"
    metrics_path = tmp_path / "metrics.json"

    result = run_compare(REFERENCE, REFERENCE, diff, metrics_path)

    assert result.returncode == 0, result.stderr
    metrics = json.loads(metrics_path.read_text(encoding="utf-8"))
    assert metrics["passed"] is True
    assert metrics["deterministic_verdict"] == "pass"
    assert metrics["dimensions"] == {
        "reference": {"width": 800, "height": 600},
        "candidate": {"width": 800, "height": 600},
    }
    assert metrics["failed_tolerances"] == []
    assert diff.is_file()
