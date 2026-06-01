from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

READY_ISSUE = """## Goal
Make PlotWidget draw one externally visible title behavior.

## PyQtGraph reference
pyqtgraph.PlotWidget title behavior

## Dependencies
none

## Owned files
- src/widgets/PlotWidget.cpp
- include/pyqtgraph/widgets/PlotWidget.h
- tests/widgets/test_plot_widget.cpp

## Scope
Only implement the title behavior described by the reference.

## TDD plan
- Add a failing widget regression test for title text.

## Validation level
numeric: required
visual: not_applicable
interaction: not_applicable

## Validation commands
- python3 -m pytest -q tests/factory

## Done definition
- The focused regression test passes.
- Changed files stay within owned files.
"""


def run_script(script: str, *args: str, input_text: str | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(ROOT / script), *args],
        input=input_text,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=ROOT,
        check=False,
    )


def test_check_issue_ready_accepts_complete_issue(tmp_path: Path) -> None:
    issue = tmp_path / "issue.md"
    issue.write_text(READY_ISSUE, encoding="utf-8")

    result = run_script("scripts/factory/check_issue_ready.py", "--issue-file", str(issue))

    assert result.returncode == 0, result.stdout + result.stderr
    payload = json.loads(result.stdout)
    assert payload["ready"] is True
    assert payload["parsed"]["owned_files"] == [
        "src/widgets/PlotWidget.cpp",
        "include/pyqtgraph/widgets/PlotWidget.h",
        "tests/widgets/test_plot_widget.cpp",
    ]


def test_check_issue_ready_rejects_unresolved_dependency(tmp_path: Path) -> None:
    issue = tmp_path / "unresolved.md"
    issue.write_text(READY_ISSUE.replace("none", "#123 unresolved", 1), encoding="utf-8")

    result = run_script("scripts/factory/check_issue_ready.py", "--issue-file", str(issue))

    assert result.returncode != 0
    payload = json.loads(result.stdout)
    assert payload["ready"] is False
    assert any("dependencies" in error for error in payload["errors"])


def test_check_issue_ready_rejects_missing_tdd_and_optional_validation(tmp_path: Path) -> None:
    issue_text = READY_ISSUE.replace("## TDD plan\n- Add a failing widget regression test for title text.\n\n", "")
    issue_text = issue_text.replace("visual: not_applicable", "visual: optional")
    issue = tmp_path / "bad.md"
    issue.write_text(issue_text, encoding="utf-8")

    result = run_script("scripts/factory/check_issue_ready.py", "--issue-file", str(issue))

    assert result.returncode != 0
    payload = json.loads(result.stdout)
    assert payload["ready"] is False
    assert any("TDD plan" in error for error in payload["errors"])
    assert any("visual" in error and "required or not_applicable" in error for error in payload["errors"])


def test_check_issue_ready_rejects_protected_files_without_marker(tmp_path: Path) -> None:
    issue = tmp_path / "protected.md"
    issue.write_text(READY_ISSUE.replace("src/widgets/PlotWidget.cpp", "WORKFLOW.md"), encoding="utf-8")

    result = run_script("scripts/factory/check_issue_ready.py", "--issue-file", str(issue))

    assert result.returncode != 0
    payload = json.loads(result.stdout)
    assert any("protected files" in error for error in payload["errors"])


def test_check_pr_scope_accepts_owned_and_small_shared_file(tmp_path: Path) -> None:
    issue = tmp_path / "issue.md"
    issue.write_text(READY_ISSUE, encoding="utf-8")

    result = run_script(
        "scripts/factory/check_pr_scope.py",
        "--issue-file",
        str(issue),
        "--changed-file",
        "src/widgets/PlotWidget.cpp",
        "--changed-file",
        "CMakeLists.txt",
    )

    assert result.returncode == 0, result.stdout + result.stderr
    payload = json.loads(result.stdout)
    assert payload["ok"] is True
    assert payload["parsed"]["classifications"]["CMakeLists.txt"] == "shared_integration"


def test_check_pr_scope_rejects_outside_and_protected_files(tmp_path: Path) -> None:
    issue = tmp_path / "issue.md"
    issue.write_text(READY_ISSUE, encoding="utf-8")

    result = run_script(
        "scripts/factory/check_pr_scope.py",
        "--issue-file",
        str(issue),
        "--changed-file",
        "README.md",
        "--changed-file",
        "WORKFLOW.md",
    )

    assert result.returncode != 0
    payload = json.loads(result.stdout)
    assert payload["ok"] is False
    assert any("outside" in error for error in payload["errors"])
    assert any("protected" in error for error in payload["errors"])


def test_apply_pr_verdict_dry_run_merge_and_no_gh_execution() -> None:
    result = run_script(
        "scripts/factory/apply_pr_verdict.py",
        input_text=json.dumps(
            {
                "pr_number": 12,
                "linked_issues": [34],
                "readiness": True,
                "evidence": True,
                "scope": True,
                "tests": True,
                "visual_required": False,
                "holdout": True,
                "risky": False,
                "fix_attempts": 0,
            }
        ),
    )

    assert result.returncode == 0, result.stdout + result.stderr
    payload = json.loads(result.stdout)
    assert payload["decision"] == "merge"
    assert payload["dry_run"] is True
    assert payload["merge_command"] is None
    assert payload["would_merge"] is True


def test_apply_pr_verdict_protected_files_force_human_review() -> None:
    result = run_script(
        "scripts/factory/apply_pr_verdict.py",
        input_text=json.dumps(
            {
                "linked_issues": [34],
                "readiness": True,
                "evidence": True,
                "scope": True,
                "tests": True,
                "holdout": True,
                "protected_files_changed": True,
                "fixable": True,
                "fix_attempts": 0,
            }
        ),
    )

    assert result.returncode != 0
    payload = json.loads(result.stdout)
    assert payload["decision"] == "human-review"
    assert any("protected files" in error for error in payload["errors"])


def test_apply_pr_verdict_fix_then_human_review() -> None:
    fixable = {
        "linked_issues": [34],
        "readiness": True,
        "evidence": False,
        "scope": True,
        "tests": False,
        "holdout": False,
        "fixable": True,
        "fix_attempts": 0,
        "max_fix_attempts": 1,
    }
    result = run_script("scripts/factory/apply_pr_verdict.py", input_text=json.dumps(fixable))
    assert result.returncode != 0
    assert json.loads(result.stdout)["decision"] == "fix"

    fixable["fix_attempts"] = 1
    result = run_script("scripts/factory/apply_pr_verdict.py", input_text=json.dumps(fixable))
    assert result.returncode != 0
    assert json.loads(result.stdout)["decision"] == "human-review"


def test_file_regression_issue_labels_ready_only_with_owned_files() -> None:
    result = run_script(
        "scripts/factory/file_regression_issue.py",
        input_text=json.dumps(
            {
                "command": "python3 -m pytest -q tests/widgets",
                "summary": "PlotWidget title regressed",
                "owned_files": ["src/widgets/PlotWidget.cpp"],
                "pyqtgraph_reference": "pyqtgraph.PlotWidget",
            }
        ),
    )
    payload = json.loads(result.stdout)
    assert result.returncode == 0
    assert payload["ready"] is True
    assert "ai:ready" in payload["labels"]
    assert "src/widgets/PlotWidget.cpp" in payload["body"]

    result = run_script("scripts/factory/file_regression_issue.py", input_text=json.dumps({"command": "ctest"}))
    payload = json.loads(result.stdout)
    assert payload["ready"] is False
    assert "ai:blocked" in payload["labels"]
    assert "human-review" in payload["labels"]


def test_file_regression_issue_runs_readiness_before_ready_label() -> None:
    result = run_script(
        "scripts/factory/file_regression_issue.py",
        input_text=json.dumps(
            {
                "command": "ctest",
                "owned_files": ["WORKFLOW.md"],
                "pyqtgraph_reference": "not_applicable",
                "visual_level": "optional",
            }
        ),
    )

    payload = json.loads(result.stdout)
    assert payload["ready"] is False
    assert "ai:blocked" in payload["labels"]
    assert "ai:ready" not in payload["labels"]
    assert payload["readiness_errors"]


def test_scope_normalizes_owned_paths(tmp_path: Path) -> None:
    issue = tmp_path / "issue.md"
    issue.write_text(READY_ISSUE.replace("src/widgets/PlotWidget.cpp", "./src/widgets/PlotWidget.cpp"), encoding="utf-8")

    result = run_script(
        "scripts/factory/check_pr_scope.py",
        "--issue-file",
        str(issue),
        "--changed-file",
        "src/widgets/PlotWidget.cpp",
    )

    assert result.returncode == 0, result.stdout + result.stderr
