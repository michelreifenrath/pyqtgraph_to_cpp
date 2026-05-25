from __future__ import annotations

import importlib.util
import json
import os
import subprocess
import sys
from pathlib import Path


from test_compare_screenshots import write_png

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts" / "check_visual_artifacts"


def run_script(*args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(SCRIPT), *args],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )


def write_pair(tmp_path: Path, actual_pixels: list[tuple[int, ...]] | None = None) -> tuple[Path, Path]:
    reference = tmp_path / "reference-source.png"
    actual = tmp_path / "actual-source.png"
    pixels = [(10, 20, 30, 255), (40, 50, 60, 255), (70, 80, 90, 255), (100, 110, 120, 255)]
    write_png(reference, 2, 2, pixels)
    write_png(actual, 2, 2, actual_pixels or pixels)
    return reference, actual


def test_P0_07_writes_canonical_visual_artifact_tree(tmp_path: Path) -> None:
    reference, actual = write_pair(tmp_path)
    reports_root = tmp_path / "reports" / "visual-diffs"

    result = run_script(
        "--case",
        "P0_07_SimplePlot",
        "--reference",
        str(reference),
        "--actual",
        str(actual),
        "--reports-root",
        str(reports_root),
        "--gpt-visual-review",
        "not_applicable",
    )

    assert result.returncode == 0, result.stderr
    case_dir = reports_root / "P0_07_SimplePlot"
    assert (case_dir / "reference.png").is_file()
    assert (case_dir / "actual.png").is_file()
    assert (case_dir / "diff.png").is_file()
    metrics_path = case_dir / "metrics.json"
    assert metrics_path.is_file()
    assert not (case_dir / "gpt5_vision_review.md").exists()

    metrics = json.loads(metrics_path.read_text(encoding="utf-8"))
    assert metrics["case"] == "P0_07_SimplePlot"
    assert metrics["dimensions"] == [2, 2]
    assert metrics["mean_abs_delta"] == 0
    assert metrics["max_delta"] == 0
    assert metrics["changed_pixel_percent"] == 0
    assert metrics["ssim"] == 1.0
    assert metrics["tolerance"] == {
        "max_mean_delta": 0.0,
        "max_pixel_delta": 0.0,
        "max_changed_percent": 0.0,
        "min_ssim": 1.0,
    }
    assert metrics["deterministic_verdict"] == "pass"
    assert metrics["passed"] is True
    assert metrics["failed_tolerances"] == []
    assert metrics["artifact_paths"] == {
        "reference": str(case_dir / "reference.png"),
        "actual": str(case_dir / "actual.png"),
        "diff": str(case_dir / "diff.png"),
        "metrics": str(case_dir / "metrics.json"),
    }
    assert metrics["review_report_path"] is None


def test_P0_07_required_gpt_review_is_copied_to_canonical_name(tmp_path: Path) -> None:
    reference, actual = write_pair(tmp_path)
    review = tmp_path / "review.md"
    review.write_text("# semantic review\n\nPASS\n", encoding="utf-8")
    reports_root = tmp_path / "reports" / "visual-diffs"

    result = run_script(
        "--case",
        "P0_07_reviewed",
        "--reference",
        str(reference),
        "--actual",
        str(actual),
        "--reports-root",
        str(reports_root),
        "--gpt-visual-review",
        "required_for_pr",
        "--review-report",
        str(review),
    )

    assert result.returncode == 0, result.stderr
    copied_review = reports_root / "P0_07_reviewed" / "gpt5_vision_review.md"
    assert copied_review.read_text(encoding="utf-8") == review.read_text(encoding="utf-8")
    metrics = json.loads((reports_root / "P0_07_reviewed" / "metrics.json").read_text(encoding="utf-8"))
    assert metrics["review_report_path"] == str(copied_review)


def test_P0_07_required_gpt_review_without_report_exits_2(tmp_path: Path) -> None:
    reference, actual = write_pair(tmp_path)

    result = run_script(
        "--case",
        "P0_07_missing_review",
        "--reference",
        str(reference),
        "--actual",
        str(actual),
        "--reports-root",
        str(tmp_path / "reports" / "visual-diffs"),
        "--gpt-visual-review",
        "required_for_pr",
    )

    assert result.returncode == 2
    assert "--review-report is required" in result.stderr


def test_P0_07_mismatch_returns_1_and_preserves_artifacts(tmp_path: Path) -> None:
    reference, actual = write_pair(
        tmp_path,
        [(10, 20, 30, 255), (90, 50, 60, 255), (70, 80, 90, 255), (100, 110, 120, 255)],
    )
    reports_root = tmp_path / "reports" / "visual-diffs"

    result = run_script(
        "--case",
        "P0_07_mismatch",
        "--reference",
        str(reference),
        "--actual",
        str(actual),
        "--reports-root",
        str(reports_root),
        "--gpt-visual-review",
        "not_applicable",
    )

    assert result.returncode == 1
    case_dir = reports_root / "P0_07_mismatch"
    for name in ("reference.png", "actual.png", "diff.png", "metrics.json"):
        assert (case_dir / name).is_file()
    metrics = json.loads((case_dir / "metrics.json").read_text(encoding="utf-8"))
    assert metrics["passed"] is False
    assert metrics["deterministic_verdict"] == "fail"
    assert metrics["failed_tolerances"]


def load_script_module():
    spec = importlib.util.spec_from_file_location("check_visual_artifacts", SCRIPT)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_P0_07_command_runner_sets_env_and_runs_reference_before_actual(tmp_path: Path) -> None:
    module = load_script_module()
    reports_root = tmp_path / "reports" / "visual-diffs"
    calls = []

    def fake_runner(command, *, cwd, env, shell, check):
        calls.append({"command": command, "cwd": cwd, "env": env, "shell": shell, "check": check})
        artifact_dir = Path(env["PG_VISUAL_ARTIFACT_DIR"])
        artifact_dir.mkdir(parents=True, exist_ok=True)
        if command == "make-reference":
            write_png(Path(env["PG_VISUAL_REFERENCE_PATH"]), 1, 1, [(1, 2, 3, 255)])
        elif command == "make-actual":
            write_png(Path(env["PG_VISUAL_ACTUAL_PATH"]), 1, 1, [(1, 2, 3, 255)])
        return subprocess.CompletedProcess(command, 0)

    result = module.main(
        [
            "--case",
            "P0_07_command",
            "--reference-command",
            "make-reference",
            "--actual-command",
            "make-actual",
            "--reports-root",
            str(reports_root),
        ],
        runner=fake_runner,
    )

    assert result == 0
    assert [call["command"] for call in calls] == ["make-reference", "make-actual"]
    case_dir = reports_root / "P0_07_command"
    for call in calls:
        assert call["cwd"] == str(ROOT)
        assert call["shell"] is True
        assert call["check"] is False
        assert call["env"]["PATH"] == os.environ["PATH"]
        assert call["env"]["PG_VISUAL_CASE"] == "P0_07_command"
        assert call["env"]["PG_VISUAL_ARTIFACT_DIR"] == str(case_dir)
        assert call["env"]["PG_VISUAL_REFERENCE_PATH"] == str(case_dir / "reference.png")
        assert call["env"]["PG_VISUAL_ACTUAL_PATH"] == str(case_dir / "actual.png")
        assert call["env"]["PG_VISUAL_DIFF_PATH"] == str(case_dir / "diff.png")
        assert call["env"]["PG_VISUAL_METRICS_PATH"] == str(case_dir / "metrics.json")
        assert call["env"]["PG_VISUAL_REVIEW_REPORT_PATH"] == str(case_dir / "gpt5_vision_review.md")


def test_P0_07_command_runner_propagates_nonzero_child_exit(tmp_path: Path) -> None:
    module = load_script_module()
    calls = []

    def fake_runner(command, *, cwd, env, shell, check):
        calls.append(command)
        if command == "make-reference":
            write_png(Path(env["PG_VISUAL_REFERENCE_PATH"]), 1, 1, [(1, 2, 3, 255)])
        return subprocess.CompletedProcess(command, 17)

    result = module.main(
        [
            "--case",
            "P0_07_command_failure",
            "--reference-command",
            "make-reference",
            "--actual-command",
            "make-actual",
            "--reports-root",
            str(tmp_path / "reports" / "visual-diffs"),
        ],
        runner=fake_runner,
    )

    assert result == 17
    assert calls == ["make-reference"]
