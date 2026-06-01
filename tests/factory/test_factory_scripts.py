from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

from scripts.factory.apply_pr_verdict import decide

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

GENERATED_ISSUE = """<!-- generated-local-issue -->
# P7.02: Port OpenGL constants helpers shaders

**Status:** todo
**Type:** AFK
**Validation class:** api-runtime
**Risk:** medium
**Blocked by:** None
**Validation authority:** local scripts/CMake/tests only; do not add GitHub Actions dependency.

## Goal
Complete OpenGL constants helpers shaders.

## Current evidence
No sufficient implementation found in the current checkout. Keep open.

## Scope
- Implement only this issue.

## Owned files
- Manifest source selectors: Qt/OpenGLConstants.py; Qt/OpenGLHelpers.py
- Manifest example selectors: none
- Repository path globs: reports/issues/P7.02/**
- Common adjuncts: focused-tests
- Changed-file rule: every modified path must match these selectors or the named adjunct set; otherwise update this issue before implementation.

## Required local proof
- Task-specific outcome: focused proof passes.

## TDD plan
- Add the smallest focused failing test first when behavior changes.

## Validation commands
- `ctest --preset dev -L P7.02`

## Acceptance criteria
- [ ] Focused proof passes.

## Done definition
- [ ] Completion report records commands, exit codes, and artifacts.

## Scope boundaries
- Edit only files allowed by `## Owned files`.
"""


def generated_issue_json(labels: list[str] | None = None, body: str = GENERATED_ISSUE) -> str:
    return json.dumps({"title": "[P7.02] Port OpenGL constants helpers shaders", "body": body, "labels": labels or ["ai:ready"]})


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


def test_check_issue_ready_accepts_generated_local_issue_with_ready_label(tmp_path: Path) -> None:
    issue = tmp_path / "generated.json"
    issue.write_text(generated_issue_json(), encoding="utf-8")

    result = run_script("scripts/factory/check_issue_ready.py", "--issue-file", str(issue))

    assert result.returncode == 0, result.stdout + result.stderr
    payload = json.loads(result.stdout)
    assert payload["ready"] is True
    assert payload["parsed"]["schema"] == "generated-local-issue"
    assert "include/pyqtgraph/Qt/OpenGLConstants.hpp" in payload["parsed"]["owned_files"]


def test_check_issue_ready_rejects_generated_local_issue_without_ready_label(tmp_path: Path) -> None:
    issue = tmp_path / "generated.json"
    issue.write_text(generated_issue_json(labels=["ai:blocked"]), encoding="utf-8")

    result = run_script("scripts/factory/check_issue_ready.py", "--issue-file", str(issue))

    assert result.returncode != 0
    payload = json.loads(result.stdout)
    assert any("ai:ready" in error for error in payload["errors"])


def test_check_issue_ready_rejects_generated_local_issue_without_selector_lines(tmp_path: Path) -> None:
    body = GENERATED_ISSUE.replace(
        "- Manifest source selectors: Qt/OpenGLConstants.py; Qt/OpenGLHelpers.py\n"
        "- Manifest example selectors: none\n"
        "- Repository path globs: reports/issues/P7.02/**\n"
        "- Common adjuncts: focused-tests\n"
        "- Changed-file rule: every modified path must match these selectors or the named adjunct set; otherwise update this issue before implementation.",
        "- src/pyqtgraph/Qt/OpenGLConstants.cpp",
    )
    issue = tmp_path / "generated.json"
    issue.write_text(generated_issue_json(body=body), encoding="utf-8")

    result = run_script("scripts/factory/check_issue_ready.py", "--issue-file", str(issue))

    assert result.returncode != 0
    payload = json.loads(result.stdout)
    assert any("owned-file selector missing" in error for error in payload["errors"])


def test_check_issue_ready_rejects_generated_local_issue_with_only_common_adjunct(tmp_path: Path) -> None:
    body = GENERATED_ISSUE.replace(
        "- Manifest source selectors: Qt/OpenGLConstants.py; Qt/OpenGLHelpers.py",
        "- Manifest source selectors: none",
    ).replace(
        "- Repository path globs: reports/issues/P7.02/**",
        "- Repository path globs: none",
    )
    issue = tmp_path / "generated.json"
    issue.write_text(generated_issue_json(body=body), encoding="utf-8")

    result = run_script("scripts/factory/check_issue_ready.py", "--issue-file", str(issue))

    assert result.returncode != 0
    payload = json.loads(result.stdout)
    assert any("at least one manifest selector" in error for error in payload["errors"])


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


def test_check_pr_scope_accepts_generated_manifest_selectors_and_adjuncts(tmp_path: Path) -> None:
    issue = tmp_path / "generated.json"
    issue.write_text(generated_issue_json(), encoding="utf-8")

    result = run_script(
        "scripts/factory/check_pr_scope.py",
        "--issue-file",
        str(issue),
        "--changed-file",
        "src/pyqtgraph/Qt/OpenGLConstants.cpp",
        "--changed-file",
        "include/pyqtgraph/Qt/OpenGLHelpers.hpp",
        "--changed-file",
        "tests/core/test_OpenGLConstants_P7_02.cpp",
        "--changed-file",
        "reports/issues/P7.02/result.md",
    )

    assert result.returncode == 0, result.stdout + result.stderr
    payload = json.loads(result.stdout)
    assert payload["ok"] is True
    assert payload["parsed"]["mode"] == "selectors"


def test_check_pr_scope_rejects_generated_protected_file_without_automation_marker(tmp_path: Path) -> None:
    issue = tmp_path / "generated.json"
    issue.write_text(generated_issue_json(), encoding="utf-8")

    result = run_script(
        "scripts/factory/check_pr_scope.py",
        "--issue-file",
        str(issue),
        "--changed-file",
        ".archon/workflows/pgcpp-fix-issue.yaml",
    )

    assert result.returncode != 0
    payload = json.loads(result.stdout)
    assert any("protected" in error for error in payload["errors"])


def mergeable_verdict_payload(**overrides):
    payload = {
        "pr_number": 12,
        "linked_issues": [34],
        "pr_state": "OPEN",
        "is_draft": False,
        "base_ref": "main",
        "head_ref": "ai/issue-34-title",
        "head_sha": "abc123",
        "auto_merge_enabled": True,
        "readiness": True,
        "evidence": True,
        "scope": True,
        "tests": True,
        "autoreview": True,
        "diff_check": True,
        "visual_required": False,
        "holdout": True,
        "risky": False,
        "protected_files_changed": False,
        "fix_attempts": 0,
    }
    payload.update(overrides)
    return payload


def test_apply_pr_verdict_dry_run_merge_and_no_gh_execution() -> None:
    result = run_script("scripts/factory/apply_pr_verdict.py", input_text=json.dumps(mergeable_verdict_payload()))

    assert result.returncode == 0, result.stdout + result.stderr
    payload = json.loads(result.stdout)
    assert payload["decision"] == "merge"
    assert payload["dry_run"] is True
    assert payload["merge_command"] == "gh pr merge 12 --squash --delete-branch --match-head-commit abc123"
    assert payload["ai_label"] == "ai:merge-ready"
    assert payload["would_label_merge_ready"] is True
    assert payload["would_merge"] is False


def test_apply_pr_verdict_allow_merge_produces_guarded_command_without_running_unit_gh() -> None:
    payload = decide(mergeable_verdict_payload(), allow_merge=True)

    assert payload["decision"] == "merge"
    assert payload["dry_run"] is False
    assert payload["would_merge"] is True
    assert payload["merge_argv"] == ["gh", "pr", "merge", "12", "--squash", "--delete-branch", "--match-head-commit", "abc123"]


def test_apply_pr_verdict_requires_gpt_visual_review_for_visual_required_merge() -> None:
    missing = decide(mergeable_verdict_payload(visual_required=True, visual=True), allow_merge=True)
    assert missing["decision"] == "human-review"
    assert any("GPT semantic visual review" in error for error in missing["errors"])

    string_false = decide(mergeable_verdict_payload(visual_required=True, visual=True, gpt_visual_review_accepted="false"), allow_merge=True)
    assert string_false["decision"] == "human-review"
    assert any("GPT semantic visual review" in error for error in string_false["errors"])

    accepted = decide(mergeable_verdict_payload(visual_required=True, visual=True, gpt_visual_review={"verdict": "pass", "recommendation": "merge_ok"}), allow_merge=True)
    assert accepted["decision"] == "merge"
    assert accepted["would_merge"] is True


def test_apply_pr_verdict_requires_oracle_and_numeric_evidence_when_declared() -> None:
    missing_oracle = decide(mergeable_verdict_payload(oracle_required=True, oracle=False), allow_merge=True)
    assert missing_oracle["decision"] == "human-review"
    assert any("oracle evidence" in error for error in missing_oracle["errors"])

    missing_numeric = decide(mergeable_verdict_payload(numeric_required=True, numeric=False), allow_merge=True)
    assert missing_numeric["decision"] == "human-review"
    assert any("numeric evidence" in error for error in missing_numeric["errors"])

    accepted = decide(mergeable_verdict_payload(oracle_required=True, oracle=True, numeric_required=True, numeric=True), allow_merge=True)
    assert accepted["decision"] == "merge"
    assert accepted["would_merge"] is True


def test_apply_pr_verdict_blocks_unsafe_pr_metadata() -> None:
    for overrides in (
        {"pr_state": "CLOSED"},
        {"is_draft": True},
        {"base_ref": "develop"},
        {"head_ref": "main"},
        {"head_ref": ""},
        {"head_sha": ""},
        {"auto_merge_enabled": False},
        {"risky": True},
        {"protected_files_changed": True},
    ):
        payload = decide(mergeable_verdict_payload(**overrides), allow_merge=True)
        assert payload["decision"] == "human-review"
        assert payload["would_merge"] is False


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
    fixable = mergeable_verdict_payload(evidence=False, tests=False, holdout=False, fixable=True, fix_attempts=0, max_fix_attempts=1)
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
                "issue_id": "P99.01",
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
    assert "<!-- generated-local-issue -->" in payload["body"]

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
                "issue_id": "P99.02",
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
