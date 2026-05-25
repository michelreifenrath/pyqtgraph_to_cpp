from __future__ import annotations

import importlib.machinery
import importlib.util
import json
import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

from test_compare_screenshots import write_png

ROOT = Path(__file__).resolve().parents[2]
REFERENCE = ROOT / "oracle" / "fixtures" / "screenshots" / "SimplePlot.reference.png"
CPP_RENDERER = ROOT / "oracle" / "scripts" / "render_cpp_example.py"
SCRIPT = ROOT / "scripts" / "check_visual_artifacts"


def run_cli(script: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(script), *args],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )


def run_script(*args: str) -> subprocess.CompletedProcess[str]:
    return run_cli(SCRIPT, *args)


def write_pair(
    tmp_path: Path, actual_pixels: list[tuple[int, ...]] | None = None
) -> tuple[Path, Path]:
    reference = tmp_path / "reference-source.png"
    actual = tmp_path / "actual-source.png"
    pixels = [
        (10, 20, 30, 255),
        (40, 50, 60, 255),
        (70, 80, 90, 255),
        (100, 110, 120, 255),
    ]
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
        "max_changed_pixel_percent": 0.0,
        "min_ssim": 1.0,
    }
    assert "max_changed_percent" not in metrics["tolerance"]
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


def test_P0_07_rejects_case_paths_outside_reports_root(tmp_path: Path) -> None:
    reference, actual = write_pair(tmp_path)
    reports_root = tmp_path / "reports" / "visual-diffs"
    escaped_parent_case = tmp_path / "reports" / "P0_07_escape"
    escaped_absolute_case = tmp_path / "absolute-case"

    for case, escaped_dir in (
        ("../P0_07_escape", escaped_parent_case),
        (str(escaped_absolute_case), escaped_absolute_case),
    ):
        result = run_script(
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
        )

        assert result.returncode == 2
        assert "--case must be a relative path inside --reports-root" in result.stderr
        assert not escaped_dir.exists()

    assert not reports_root.exists()


def test_P0_07_required_gpt_review_is_copied_to_canonical_name(tmp_path: Path) -> None:
    reference, actual = write_pair(tmp_path)
    review = tmp_path / "review.md"
    review.write_text(
        "verdict: pass\n"
        "blocking_differences: []\n"
        "non_blocking_differences: []\n"
        "likely_causes: []\n"
        "recommendation: merge_ok\n",
        encoding="utf-8",
    )
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
    assert copied_review.read_text(encoding="utf-8") == review.read_text(
        encoding="utf-8"
    )
    metrics = json.loads(
        (reports_root / "P0_07_reviewed" / "metrics.json").read_text(encoding="utf-8")
    )
    assert metrics["review_report_path"] == str(copied_review)
    assert metrics["semantic_review"] == {
        "mode": "required_for_pr",
        "verdict": "pass",
        "recommendation": "merge_ok",
        "accepted": True,
        "blocks_gate": False,
        "failure_reason": None,
    }


@pytest.mark.parametrize(
    ("review_text", "expected_verdict", "expected_recommendation"),
    [
        ("verdict: fail\nrecommendation: needs_fix\n", "fail", "needs_fix"),
        (
            "verdict: uncertain\nrecommendation: human_review\n",
            "uncertain",
            "human_review",
        ),
        ("verdict: pass\nrecommendation: human_review\n", "pass", "human_review"),
        ("recommendation: merge_ok\n", None, "merge_ok"),
    ],
)
def test_P0_07_required_gpt_review_non_pass_blocks_gate(
    tmp_path: Path,
    review_text: str,
    expected_verdict: str | None,
    expected_recommendation: str,
) -> None:
    reference, actual = write_pair(tmp_path)
    review = tmp_path / "review.md"
    review.write_text(review_text, encoding="utf-8")
    reports_root = tmp_path / "reports" / "visual-diffs"

    result = run_script(
        "--case",
        "P0_07_review_blocks_gate",
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

    assert result.returncode == 1, result.stdout
    case_dir = reports_root / "P0_07_review_blocks_gate"
    copied_review = case_dir / "gpt5_vision_review.md"
    assert copied_review.read_text(encoding="utf-8") == review_text
    metrics = json.loads((case_dir / "metrics.json").read_text(encoding="utf-8"))
    assert json.loads(result.stdout) == metrics
    assert metrics["passed"] is False
    assert metrics["deterministic_verdict"] == "pass"
    assert metrics["failed_tolerances"] == []
    assert metrics["failed_checks"] == ["gpt_visual_review"]
    assert metrics["semantic_review"] == {
        "mode": "required_for_pr",
        "verdict": expected_verdict,
        "recommendation": expected_recommendation,
        "accepted": False,
        "blocks_gate": True,
        "failure_reason": (
            "required GPT visual review must report verdict: pass and "
            "recommendation: merge_ok"
        ),
    }


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
    loader = importlib.machinery.SourceFileLoader("check_visual_artifacts", str(SCRIPT))
    spec = importlib.util.spec_from_loader(loader.name, loader)
    assert spec is not None
    module = importlib.util.module_from_spec(spec)
    loader.exec_module(module)
    return module


def noisy_copy_command(source: Path, target_env: str, message: str) -> str:
    code = (
        "import os, shutil; "
        f"print({message!r}); "
        f"shutil.copyfile({str(source)!r}, os.environ[{target_env!r}])"
    )
    return f"{shlex.quote(sys.executable)} -c {shlex.quote(code)}"


def test_P0_07_command_runner_keeps_child_stdout_out_of_metrics_json(
    tmp_path: Path,
) -> None:
    reference, actual = write_pair(tmp_path)
    reports_root = tmp_path / "reports" / "visual-diffs"

    result = run_script(
        "--case",
        "P0_07_noisy_command",
        "--reference-command",
        noisy_copy_command(
            reference, "PG_VISUAL_REFERENCE_PATH", "reference child stdout"
        ),
        "--actual-command",
        noisy_copy_command(actual, "PG_VISUAL_ACTUAL_PATH", "actual child stdout"),
        "--reports-root",
        str(reports_root),
    )

    assert result.returncode == 0, result.stderr
    metrics = json.loads(result.stdout)
    assert "reference child stdout" not in result.stdout
    assert "actual child stdout" not in result.stdout
    assert metrics == json.loads(
        (reports_root / "P0_07_noisy_command" / "metrics.json").read_text(
            encoding="utf-8"
        )
    )


def test_P0_07_command_runner_rejects_stale_outputs_when_children_write_nothing(
    tmp_path: Path,
) -> None:
    reports_root = tmp_path / "reports" / "visual-diffs"
    case_dir = reports_root / "P0_07_stale_command"
    case_dir.mkdir(parents=True)
    write_png(case_dir / "reference.png", 1, 1, [(1, 2, 3, 255)])
    write_png(case_dir / "actual.png", 1, 1, [(1, 2, 3, 255)])

    result = run_script(
        "--case",
        "P0_07_stale_command",
        "--reference-command",
        "true",
        "--actual-command",
        "true",
        "--reports-root",
        str(reports_root),
    )

    assert result.returncode == 2
    assert "reference command did not create expected artifact" in result.stderr
    assert not (case_dir / "reference.png").exists()
    assert not (case_dir / "actual.png").exists()
    assert not (case_dir / "metrics.json").exists()


def test_P0_07_command_runner_rejects_missing_actual_output(
    tmp_path: Path, capsys
) -> None:
    module = load_script_module()
    reports_root = tmp_path / "reports" / "visual-diffs"
    calls = []

    def fake_runner(command, *, cwd, env, shell, check, stdout, stderr):
        calls.append(command)
        if command == "make-reference":
            write_png(Path(env["PG_VISUAL_REFERENCE_PATH"]), 1, 1, [(1, 2, 3, 255)])
            write_png(Path(env["PG_VISUAL_ACTUAL_PATH"]), 1, 1, [(1, 2, 3, 255)])
        return subprocess.CompletedProcess(command, 0)

    result = module.main(
        [
            "--case",
            "P0_07_missing_actual",
            "--reference-command",
            "make-reference",
            "--actual-command",
            "make-actual",
            "--reports-root",
            str(reports_root),
        ],
        runner=fake_runner,
    )

    captured = capsys.readouterr()
    case_dir = reports_root / "P0_07_missing_actual"
    assert result == 2
    assert "actual command did not create expected artifact" in captured.err
    assert calls == ["make-reference", "make-actual"]
    assert (case_dir / "reference.png").is_file()
    assert not (case_dir / "actual.png").exists()
    assert not (case_dir / "metrics.json").exists()


def test_P0_07_command_runner_sets_env_and_runs_reference_before_actual(
    tmp_path: Path,
) -> None:
    module = load_script_module()
    reports_root = tmp_path / "reports" / "visual-diffs"
    calls = []

    def fake_runner(command, *, cwd, env, shell, check, stdout, stderr):
        calls.append(
            {
                "command": command,
                "cwd": cwd,
                "env": env,
                "shell": shell,
                "check": check,
                "stdout": stdout,
                "stderr": stderr,
            }
        )
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
        assert call["stdout"] == subprocess.PIPE
        assert call["stderr"] == subprocess.PIPE
        assert call["env"]["PATH"] == os.environ["PATH"]
        assert call["env"]["PG_VISUAL_CASE"] == "P0_07_command"
        assert call["env"]["PG_VISUAL_ARTIFACT_DIR"] == str(case_dir)
        assert call["env"]["PG_VISUAL_REFERENCE_PATH"] == str(
            case_dir / "reference.png"
        )
        assert call["env"]["PG_VISUAL_ACTUAL_PATH"] == str(case_dir / "actual.png")
        assert call["env"]["PG_VISUAL_DIFF_PATH"] == str(case_dir / "diff.png")
        assert call["env"]["PG_VISUAL_METRICS_PATH"] == str(case_dir / "metrics.json")
        assert call["env"]["PG_VISUAL_REVIEW_REPORT_PATH"] == str(
            case_dir / "gpt5_vision_review.md"
        )


def test_P0_07_command_runner_propagates_nonzero_child_exit(tmp_path: Path) -> None:
    module = load_script_module()
    calls = []

    def fake_runner(command, *, cwd, env, shell, check, stdout, stderr):
        assert stdout == subprocess.PIPE
        assert stderr == subprocess.PIPE
        calls.append(command)
        if command == "make-reference":
            write_png(Path(env["PG_VISUAL_REFERENCE_PATH"]), 1, 1, [(1, 2, 3, 255)])
            return subprocess.CompletedProcess(command, 0)
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
    assert calls == ["make-reference", "make-actual"]


def test_P0_07_default_reports_root_is_repo_anchored(
    tmp_path: Path, monkeypatch
) -> None:
    module = load_script_module()
    process_cwd = tmp_path / "process-cwd"
    command_cwd = tmp_path / "command-cwd"
    process_cwd.mkdir()
    command_cwd.mkdir()
    monkeypatch.chdir(process_cwd)
    case_name = "P0_07_default_reports_root"
    case_dir = ROOT / "reports" / "visual-diffs" / case_name
    calls = []

    def fake_runner(command, *, cwd, env, shell, check, stdout, stderr):
        calls.append({"command": command, "cwd": cwd, "env": env})
        Path(env["PG_VISUAL_ARTIFACT_DIR"]).mkdir(parents=True, exist_ok=True)
        if command == "make-reference":
            write_png(Path(env["PG_VISUAL_REFERENCE_PATH"]), 1, 1, [(1, 2, 3, 255)])
        elif command == "make-actual":
            write_png(Path(env["PG_VISUAL_ACTUAL_PATH"]), 1, 1, [(1, 2, 3, 255)])
        return subprocess.CompletedProcess(command, 0)

    shutil.rmtree(case_dir, ignore_errors=True)
    try:
        result = module.main(
            [
                "--case",
                case_name,
                "--reference-command",
                "make-reference",
                "--actual-command",
                "make-actual",
                "--cwd",
                str(command_cwd),
            ],
            runner=fake_runner,
        )

        assert result == 0
        assert (case_dir / "reference.png").is_file()
        assert (case_dir / "actual.png").is_file()
        assert (case_dir / "diff.png").is_file()
        assert (case_dir / "metrics.json").is_file()
        assert not (process_cwd / "reports").exists()
        assert not (command_cwd / "reports").exists()
        assert [call["command"] for call in calls] == ["make-reference", "make-actual"]
        for call in calls:
            assert call["cwd"] == str(command_cwd)
            assert call["env"]["PG_VISUAL_ARTIFACT_DIR"] == str(case_dir)
            assert call["env"]["PG_VISUAL_REFERENCE_PATH"] == str(
                case_dir / "reference.png"
            )
            assert call["env"]["PG_VISUAL_ACTUAL_PATH"] == str(case_dir / "actual.png")
    finally:
        shutil.rmtree(case_dir, ignore_errors=True)


def test_P0_07_command_runner_exports_absolute_artifacts_for_custom_cwd(
    tmp_path: Path, monkeypatch
) -> None:
    module = load_script_module()
    process_cwd = tmp_path / "process-cwd"
    command_cwd = tmp_path / "command-cwd"
    process_cwd.mkdir()
    command_cwd.mkdir()
    monkeypatch.chdir(process_cwd)
    calls = []

    def child_path(cwd: str, raw_path: str) -> Path:
        path = Path(raw_path)
        return path if path.is_absolute() else Path(cwd) / path

    def fake_runner(command, *, cwd, env, shell, check, stdout, stderr):
        calls.append(
            {
                "command": command,
                "cwd": cwd,
                "env": env,
                "shell": shell,
                "check": check,
                "stdout": stdout,
                "stderr": stderr,
            }
        )
        artifact_dir = child_path(cwd, env["PG_VISUAL_ARTIFACT_DIR"])
        artifact_dir.mkdir(parents=True, exist_ok=True)
        if command == "make-reference":
            write_png(
                child_path(cwd, env["PG_VISUAL_REFERENCE_PATH"]),
                1,
                1,
                [(1, 2, 3, 255)],
            )
        elif command == "make-actual":
            write_png(
                child_path(cwd, env["PG_VISUAL_ACTUAL_PATH"]),
                1,
                1,
                [(1, 2, 3, 255)],
            )
        return subprocess.CompletedProcess(command, 0)

    result = module.main(
        [
            "--case",
            "P0_07_command_custom_cwd",
            "--reference-command",
            "make-reference",
            "--actual-command",
            "make-actual",
            "--reports-root",
            "reports/visual-diffs",
            "--cwd",
            str(command_cwd),
        ],
        runner=fake_runner,
    )

    assert result == 0
    case_dir = process_cwd / "reports" / "visual-diffs" / "P0_07_command_custom_cwd"
    assert (case_dir / "reference.png").is_file()
    assert (case_dir / "actual.png").is_file()
    assert (case_dir / "diff.png").is_file()
    assert (case_dir / "metrics.json").is_file()
    assert not (command_cwd / "reports").exists()
    assert [call["command"] for call in calls] == ["make-reference", "make-actual"]
    for call in calls:
        assert call["cwd"] == str(command_cwd)
        assert call["stdout"] == subprocess.PIPE
        assert call["stderr"] == subprocess.PIPE
        assert call["env"]["PG_VISUAL_ARTIFACT_DIR"] == str(case_dir)
        assert call["env"]["PG_VISUAL_REFERENCE_PATH"] == str(
            case_dir / "reference.png"
        )
        assert call["env"]["PG_VISUAL_ACTUAL_PATH"] == str(case_dir / "actual.png")


def test_P0_07_simpleplot_smoke_writes_canonical_artifacts(tmp_path: Path) -> None:
    reports_root = tmp_path / "reports" / "visual-diffs"
    case_dir = reports_root / "P0_07_SimplePlot_smoke"
    rendered_actual = tmp_path / "SimplePlot.actual.png"

    render_result = run_cli(
        CPP_RENDERER,
        "SimplePlot",
        "--output",
        str(rendered_actual),
        "--width",
        "800",
        "--height",
        "600",
    )

    assert render_result.returncode == 0, render_result.stderr
    render_status = json.loads(render_result.stdout)
    assert render_status["example"] == "SimplePlot"
    assert render_status["dimensions"] == {"width": 800, "height": 600}
    assert render_status["placeholder"] is True

    checker_result = run_script(
        "--case",
        "P0_07_SimplePlot_smoke",
        "--reference",
        str(REFERENCE),
        "--actual",
        str(rendered_actual),
        "--reports-root",
        str(reports_root),
        "--gpt-visual-review",
        "not_applicable",
    )

    assert checker_result.returncode == 1, checker_result.stderr
    assert checker_result.stderr == ""
    for name in ("reference.png", "actual.png", "diff.png", "metrics.json"):
        assert (case_dir / name).is_file()
    metrics = json.loads((case_dir / "metrics.json").read_text(encoding="utf-8"))
    assert json.loads(checker_result.stdout) == metrics
    assert metrics["passed"] is False
    assert metrics["deterministic_verdict"] == "fail"
    assert metrics["dimensions"] == [800, 600]
    assert metrics["artifact_paths"] == {
        "reference": str(case_dir / "reference.png"),
        "actual": str(case_dir / "actual.png"),
        "diff": str(case_dir / "diff.png"),
        "metrics": str(case_dir / "metrics.json"),
    }
    assert metrics["review_report_path"] is None
    assert metrics["changed_pixel_percent"] > 0
    assert metrics["mean_abs_delta"] > 0
    assert metrics["max_delta"] > 0
    assert metrics["failed_tolerances"]


def test_P0_07_min_ssim_gate_uses_structural_similarity_not_mean_delta(
    tmp_path: Path,
) -> None:
    reference, actual = write_pair(
        tmp_path,
        [
            (255, 255, 255, 255),
            (0, 0, 0, 255),
            (255, 255, 255, 255),
            (0, 0, 0, 255),
        ],
    )
    write_png(
        reference,
        2,
        2,
        [
            (0, 0, 0, 255),
            (255, 255, 255, 255),
            (0, 0, 0, 255),
            (255, 255, 255, 255),
        ],
    )
    reports_root = tmp_path / "reports" / "visual-diffs"

    result = run_script(
        "--case",
        "P0_07_ssim_inversion",
        "--reference",
        str(reference),
        "--actual",
        str(actual),
        "--reports-root",
        str(reports_root),
        "--gpt-visual-review",
        "not_applicable",
        "--max-mean-delta",
        "255",
        "--max-pixel-delta",
        "255",
        "--max-changed-percent",
        "100",
        "--min-ssim",
        "0.1",
    )

    assert result.returncode == 1, result.stdout
    metrics = json.loads(
        (reports_root / "P0_07_ssim_inversion" / "metrics.json").read_text(
            encoding="utf-8"
        )
    )
    mean_delta_surrogate = 1.0 - (metrics["mean_abs_delta"] / 255.0)
    assert metrics["ssim"] < 0.1
    assert abs(metrics["ssim"] - mean_delta_surrogate) > 0.1
    assert metrics["passed"] is False
    assert metrics["deterministic_verdict"] == "fail"
    assert "ssim" in metrics["failed_tolerances"]
