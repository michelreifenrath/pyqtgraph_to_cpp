from pathlib import Path
from typing import Any

import pytest

from automation.pi_symphony import runner
from automation.pi_symphony.config import LoadedWorkflow, WorkflowConfig
from automation.pi_symphony.github import Issue
from automation.pi_symphony.process import CommandResult, run
from automation.pi_symphony.runner import GateFailure, _commit_worktree_changes_for_review, _ensure_descriptive_review_commit, _handle_phase_failure, _pi_command, _pr_body
from automation.pi_symphony.state import update_issue
from automation.pi_symphony.workspace import diff_stats


def test_pi_command_includes_configured_codex_model_and_thinking():
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": "/home/michel/code/ai-workspaces/pyqtgraph_to_cpp"},
            "pi": {
                "provider": "openai-codex",
                "model": "gpt-5.5",
                "implementation_thinking": "xhigh",
                "use_subagents": True,
            },
        },
        body="body",
    )

    assert _pi_command(config, thinking=config.pi.implementation_thinking, prompt="do work") == [
        "pi",
        "--provider",
        "openai-codex",
        "--model",
        "gpt-5.5",
        "--thinking",
        "xhigh",
        "--print",
        "--no-session",
        "do work",
    ]


def test_intake_skips_ready_issue_with_unmet_local_id_dependency(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "owner/repo"},
            "workspace": {"root": str(tmp_path / "workspaces")},
        },
        body="body",
    )
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    ready = [
        Issue(214, "[P10.06] Run final acceptance", "**Blocked by:** P10.05\n", ["ai:ready"], "url", "michel"),
        Issue(95, "[P0.01] Define parity contract", "**Blocked by:** None\n", ["ai:ready"], "url", "michel"),
    ]
    all_issues = [
        {"number": 214, "title": "[P10.06] Run final acceptance", "state": "open", "labels": []},
        {"number": 213, "title": "[P10.05] Complete final review", "state": "open", "labels": []},
        {"number": 95, "title": "[P0.01] Define parity contract", "state": "open", "labels": []},
    ]

    monkeypatch.setattr(runner, "ensure_runtime_prereqs", lambda _config: [])
    monkeypatch.setattr(runner, "list_ready_issues", lambda _config, limit=None: ready)
    monkeypatch.setattr(runner, "list_issue_items", lambda _config: all_issues)
    monkeypatch.setattr(runner, "get_issue", lambda _repo_root, _issue_number: {})

    result = runner.intake(loaded, dry_run=True)

    assert {action["issue"]: action["action"] for action in result["actions"]} == {
        214: "skipped_dependencies",
        95: "would_claim",
    }
    assert result["actions"][0]["unmet_dependencies"] == ["P10.05"]


def test_promote_unblocked_issue_moves_dependency_satisfied_blocker_to_ready(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "owner/repo"},
            "workspace": {"root": str(tmp_path / "workspaces")},
        },
        body="body",
    )
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    all_issues = [
        {
            "number": 96,
            "title": "[P0.02] Build manifest",
            "body": "**Blocked by:** P0.01\n",
            "state": "open",
            "labels": [{"name": "ai:blocked"}],
            "html_url": "url",
            "user": {"login": "michel"},
        },
        {"number": 95, "title": "[P0.01] Define parity", "body": "", "state": "closed", "labels": []},
    ]
    added: list[tuple[int, list[str]]] = []
    removed: list[tuple[int, list[str]]] = []

    monkeypatch.setattr(runner, "list_issue_items", lambda _config: all_issues)
    monkeypatch.setattr(runner, "add_labels", lambda _config, number, labels: added.append((number, labels)))
    monkeypatch.setattr(runner, "remove_labels", lambda _config, number, labels: removed.append((number, labels)))

    result = runner.promote_unblocked_issues(loaded)

    assert result == {"actions": [{"issue": 96, "action": "promoted_ready"}]}
    assert added == [(96, ["ai:ready"])]
    assert removed == [(96, ["ai:blocked"])]



def test_promote_unblocked_issue_leaves_human_review_blocked(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "owner/repo"},
            "workspace": {"root": str(tmp_path / "workspaces")},
        },
        body="body",
    )
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    all_issues = [
        {
            "number": 96,
            "title": "[P0.02] Build manifest",
            "body": "**Blocked by:** None\n",
            "state": "open",
            "labels": [{"name": "ai:blocked"}, {"name": "human-review"}],
            "html_url": "url",
            "user": {"login": "michel"},
        },
    ]

    monkeypatch.setattr(runner, "list_issue_items", lambda _config: all_issues)

    assert runner.promote_unblocked_issues(loaded) == {"actions": []}



def test_intake_allows_ready_issue_when_local_dependency_is_closed(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "owner/repo"},
            "workspace": {"root": str(tmp_path / "workspaces")},
        },
        body="body",
    )
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    ready = [Issue(96, "[P0.02] Build manifest", "**Blocked by:** P0.01\n", ["ai:ready"], "url", "michel")]
    all_issues = [
        {"number": 96, "title": "[P0.02] Build manifest", "state": "open", "labels": []},
        {"number": 95, "title": "[P0.01] Define parity contract", "state": "closed", "labels": []},
    ]

    monkeypatch.setattr(runner, "ensure_runtime_prereqs", lambda _config: [])
    monkeypatch.setattr(runner, "list_ready_issues", lambda _config, limit=None: ready)
    monkeypatch.setattr(runner, "list_issue_items", lambda _config: all_issues)
    monkeypatch.setattr(runner, "get_issue", lambda _repo_root, _issue_number: {})

    result = runner.intake(loaded, dry_run=True)

    assert result["actions"] == [{"issue": 96, "action": "would_claim"}]


def test_intake_blocks_phase_range_until_all_phase_issues_are_closed(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "owner/repo"},
            "workspace": {"root": str(tmp_path / "workspaces")},
        },
        body="body",
    )
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    ready = [Issue(209, "[P10.01] Achieve full manifest closure", "**Blocked by:** P2-P9\n", ["ai:ready"], "url", "michel")]
    all_issues = [
        {"number": 209, "title": "[P10.01] Achieve full manifest closure", "state": "open", "labels": []},
        {"number": 120, "title": "[P2.01] Closed phase issue", "state": "closed", "labels": []},
        {"number": 180, "title": "[P7.01] Still open phase issue", "state": "open", "labels": []},
    ]

    monkeypatch.setattr(runner, "ensure_runtime_prereqs", lambda _config: [])
    monkeypatch.setattr(runner, "list_ready_issues", lambda _config, limit=None: ready)
    monkeypatch.setattr(runner, "list_issue_items", lambda _config: all_issues)
    monkeypatch.setattr(runner, "get_issue", lambda _repo_root, _issue_number: {})

    result = runner.intake(loaded, dry_run=True)

    assert result["actions"] == [
        {"issue": 209, "action": "skipped_dependencies", "unmet_dependencies": ["P7.01"]}
    ]


def test_review_commit_makes_untracked_new_files_visible_to_branch_diff(tmp_path: Path):
    repo = tmp_path / "repo"
    repo.mkdir()
    run(["git", "init", "-b", "main"], cwd=repo, timeout=60)
    run(["git", "config", "user.email", "pi-symphony@example.invalid"], cwd=repo, timeout=60)
    run(["git", "config", "user.name", "Pi Symphony"], cwd=repo, timeout=60)
    (repo / "README.md").write_text("base\n", encoding="utf-8")
    run(["git", "add", "README.md"], cwd=repo, timeout=60)
    run(["git", "commit", "-m", "base"], cwd=repo, timeout=60)
    run(["git", "update-ref", "refs/remotes/origin/main", "HEAD"], cwd=repo, timeout=60)

    new_file = repo / "oracle" / "new_tool.py"
    new_file.parent.mkdir()
    new_file.write_text("print('new')\n", encoding="utf-8")

    issue = Issue(16, "[AI] PGORACLE-006: Add SimplePlot numeric oracle", "body", ["tag:oracle"], "url", "michel")

    commit = _commit_worktree_changes_for_review(
        repo,
        issue=issue,
        validations=[{"command": "scripts/gate focus PGORACLE-006", "returncode": 0}],
    )

    assert commit is not None
    assert run(["git", "status", "--short"], cwd=repo, timeout=60).stdout.strip() == ""
    assert run(["git", "diff", "--name-only", "origin/main...HEAD"], cwd=repo, timeout=60).stdout.splitlines() == ["oracle/new_tool.py"]
    message = run(["git", "log", "-1", "--pretty=%B"], cwd=repo, timeout=60).stdout
    assert message.startswith("feat(simpleplot): add SimplePlot numeric oracle\n")
    assert "Addresses #16." in message
    assert "What changed:\n- Add SimplePlot numeric oracle." in message
    assert "Validation:\n- scripts/gate focus PGORACLE-006: passed" in message


def test_review_phase_amends_legacy_generic_review_commit(tmp_path: Path):
    repo = tmp_path / "repo"
    repo.mkdir()
    run(["git", "init", "-b", "main"], cwd=repo, timeout=60)
    run(["git", "config", "user.email", "pi-symphony@example.invalid"], cwd=repo, timeout=60)
    run(["git", "config", "user.name", "Pi Symphony"], cwd=repo, timeout=60)
    (repo / "README.md").write_text("base\n", encoding="utf-8")
    run(["git", "add", "README.md"], cwd=repo, timeout=60)
    run(["git", "commit", "-m", "base"], cwd=repo, timeout=60)
    (repo / "README.md").write_text("base\nchange\n", encoding="utf-8")
    run(
        [
            "git",
            "commit",
            "-am",
            "fix: address issue #41",
            "-m",
            "Automated Pi Symphony implementation.",
        ],
        cwd=repo,
        timeout=60,
    )
    old_head = run(["git", "rev-parse", "HEAD"], cwd=repo, timeout=60).stdout.strip()
    issue = Issue(41, "[AI] PGPLOT-004: Implement PlotCurveItem QPainter paint path", "body", ["tag:plot"], "url", "michel")

    new_head = _ensure_descriptive_review_commit(
        repo,
        issue=issue,
        validations=[{"command": "python3 -m pytest -q", "returncode": 0}],
    )

    assert new_head is not None
    assert new_head != old_head
    message = run(["git", "log", "-1", "--pretty=%B"], cwd=repo, timeout=60).stdout
    assert message.startswith("feat(plotcurveitem): implement PlotCurveItem QPainter paint path\n")
    assert "Automated Pi Symphony implementation" not in message
    assert "Validation:\n- python3 -m pytest -q: passed" in message


def test_pi_prompt_renders_configured_implementation_template(tmp_path: Path):
    template = tmp_path / "prompts" / "implement-ticket.md"
    template.parent.mkdir()
    template.write_text(
        "Repo={{ repo }} Issue=#{{ issue_number }} {{ issue_title }} Branch={{ branch }} Body={{ issue_body }} Workflow={{ workflow_body }}",
        encoding="utf-8",
    )
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "owner/repo"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "prompts": {"implement": "prompts/implement-ticket.md"},
        },
        body="workflow contract",
    )
    issue = Issue(7, "[AI] Add widget", "Goal body", ["tag:plot"], "url", "michel")

    prompt = runner._pi_prompt(config, issue, "ai/issue-7-add-widget", repo_root=tmp_path)

    assert "Repo=owner/repo" in prompt
    assert "Issue=#7 [AI] Add widget" in prompt
    assert "Branch=ai/issue-7-add-widget" in prompt
    assert "Body=Goal body" in prompt
    assert "Workflow=workflow contract" in prompt


def test_run_autoreview_uses_configured_review_context_prompt(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    autoreview = tmp_path / "autoreview"
    autoreview.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
    autoreview.chmod(0o755)
    review_prompt = tmp_path / "prompts" / "review-context.md"
    review_prompt.parent.mkdir()
    review_prompt.write_text("Review repo {{ repo }} under {{ workflow_body }}.", encoding="utf-8")
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "owner/repo"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "autoreview": {"command": str(autoreview)},
            "prompts": {"review_context": "prompts/review-context.md"},
        },
        body="lean contract",
    )
    captured: dict[str, str] = {}

    def fake_run(args, **_kwargs):
        prompt_file = Path(args[args.index("--prompt-file") + 1])
        captured["prompt"] = prompt_file.read_text(encoding="utf-8")
        return CommandResult(args=args, returncode=0, stdout="ok", stderr="")

    monkeypatch.setattr(runner, "run", fake_run)

    result = runner.run_autoreview(config, tmp_path, tmp_path)

    assert result["engine"] == "autoreview"
    assert captured["prompt"] == "Review repo owner/repo under lean contract."


def test_mandatory_autoreview_does_not_fallback_to_codex(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "owner/repo"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "autoreview": {"command": "missing-autoreview", "mandatory_gate": True},
        },
        body="body",
    )
    monkeypatch.setattr(runner.shutil, "which", lambda command: "/usr/bin/codex" if command == "codex" else None)
    monkeypatch.setattr(runner, "run", lambda *_args, **_kwargs: (_ for _ in ()).throw(AssertionError("codex fallback must not run for mandatory autoreview")))

    with pytest.raises(GateFailure, match="mandatory autoreview"):
        runner.run_autoreview(config, tmp_path, tmp_path)


def test_release_issue_requires_autoreview_evidence_before_pr(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "validation": {"commands": []},
        },
        body="body",
    )
    worktree = Path(config.workspace.root) / "issue-16"
    worktree.mkdir(parents=True)
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)

    monkeypatch.setattr(runner, "view_issue", lambda _config, _issue_number: Issue(16, "new files", "body", [], "url", "michel"))
    monkeypatch.setattr(runner, "get_issue", lambda _repo_root, _issue_number: {"status": "reviewed", "branch": "ai/issue-16-new-files", "reviewed_head": "reviewed-sha"})
    monkeypatch.setattr(runner, "run_validations", lambda *_args, **_kwargs: (_ for _ in ()).throw(AssertionError("release should fail before validation without autoreview evidence")))

    with pytest.raises(GateFailure, match="autoreview evidence"):
        runner.release_issue(loaded, 16)


def test_diff_stats_uses_branch_diff_not_upstream_two_dot_changes(tmp_path: Path):
    repo = tmp_path / "repo"
    repo.mkdir()
    run(["git", "init", "-b", "main"], cwd=repo, timeout=60)
    run(["git", "config", "user.email", "pi-symphony@example.invalid"], cwd=repo, timeout=60)
    run(["git", "config", "user.name", "Pi Symphony"], cwd=repo, timeout=60)
    (repo / "README.md").write_text("base\n", encoding="utf-8")
    run(["git", "add", "README.md"], cwd=repo, timeout=60)
    run(["git", "commit", "-m", "base"], cwd=repo, timeout=60)
    run(["git", "update-ref", "refs/remotes/origin/main", "HEAD"], cwd=repo, timeout=60)
    run(["git", "checkout", "-b", "feature"], cwd=repo, timeout=60)
    run(["git", "checkout", "main"], cwd=repo, timeout=60)
    (repo / "README.md").write_text("base\nupstream\n", encoding="utf-8")
    run(["git", "commit", "-am", "upstream"], cwd=repo, timeout=60)
    run(["git", "update-ref", "refs/remotes/origin/main", "HEAD"], cwd=repo, timeout=60)
    run(["git", "checkout", "feature"], cwd=repo, timeout=60)

    assert diff_stats(repo, "origin/main") == ([], 0)


def test_review_issue_refuses_empty_branch_diff_before_autoreview(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "validation": {"commands": []},
        },
        body="body",
    )
    worktree = Path(config.workspace.root) / "issue-16"
    worktree.mkdir(parents=True)
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)

    monkeypatch.setattr(runner, "view_issue", lambda _config, _issue_number: Issue(16, "new files", "body", [], "url", "michel"))
    monkeypatch.setattr(runner, "run_validations", lambda _config, _worktree, _issue_log_dir: [])
    monkeypatch.setattr(runner, "diff_stats", lambda _worktree, _base: ([], 0))
    monkeypatch.setattr(runner, "run_autoreview", lambda *_args, **_kwargs: (_ for _ in ()).throw(AssertionError("autoreview must not run for empty diff")))

    with pytest.raises(GateFailure, match="review diff is empty"):
        runner.review_issue(loaded, 16)


def test_review_issue_allows_large_verified_generated_manifest_diff(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "validation": {"commands": []},
            "policy": {
                "max_changed_files_without_human_review": 3,
                "max_diff_lines_without_human_review": 1000,
                "generated_diff_exceptions": [
                    {
                        "path": "port_manifest.yaml",
                        "verify_command": "python3 oracle/scripts/generate_class_inventory.py --check",
                    }
                ],
            },
        },
        body="body",
    )
    worktree = Path(config.workspace.root) / "issue-10"
    worktree.mkdir(parents=True)
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    labels: list[str] = []
    reviewed: list[bool] = []

    monkeypatch.setattr(runner, "view_issue", lambda _config, _issue_number: Issue(10, "inventory", "body", [], "url", "michel"))
    monkeypatch.setattr(runner, "run_validations", lambda _config, _worktree, _issue_log_dir: [])
    monkeypatch.setattr(runner, "diff_stats", lambda _worktree, _base: (["oracle/scripts/generate_class_inventory.py", "port_manifest.yaml", "reports/agents/PGINV-003.md", "tests/oracle/test_class_inventory.py"], 3819))
    monkeypatch.setattr(
        runner,
        "diff_file_stats",
        lambda _worktree, _base: [
            {"path": "oracle/scripts/generate_class_inventory.py", "changed_lines": 339},
            {"path": "port_manifest.yaml", "changed_lines": 3047},
            {"path": "reports/agents/PGINV-003.md", "changed_lines": 30},
            {"path": "tests/oracle/test_class_inventory.py", "changed_lines": 403},
        ],
    )
    monkeypatch.setattr(runner, "add_labels", lambda _config, _number, values: labels.extend(values))
    monkeypatch.setattr(runner, "run_autoreview", lambda *_args, **_kwargs: reviewed.append(True) or {"status": "passed"})
    monkeypatch.setattr(runner, "run", lambda args, **_kwargs: CommandResult(args=args, returncode=0, stdout="class inventory verified (227 classes)\n", stderr=""))

    result = runner.review_issue(loaded, 10)

    assert reviewed == [True]
    assert labels == []
    assert result["changed_lines"] == 3819
    assert result["review_surface_lines"] == 772
    assert result["verified_generated_files"] == ["port_manifest.yaml"]


def test_review_issue_blocks_large_generated_diff_when_verification_fails(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "validation": {"commands": []},
            "policy": {
                "max_changed_files_without_human_review": 3,
                "max_diff_lines_without_human_review": 1000,
                "generated_diff_exceptions": [
                    {
                        "path": "port_manifest.yaml",
                        "verify_command": "python3 oracle/scripts/generate_class_inventory.py --check",
                    }
                ],
            },
        },
        body="body",
    )
    worktree = Path(config.workspace.root) / "issue-10"
    worktree.mkdir(parents=True)
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    labels: list[str] = []

    monkeypatch.setattr(runner, "view_issue", lambda _config, _issue_number: Issue(10, "inventory", "body", [], "url", "michel"))
    monkeypatch.setattr(runner, "run_validations", lambda _config, _worktree, _issue_log_dir: [])
    monkeypatch.setattr(runner, "diff_stats", lambda _worktree, _base: (["oracle/scripts/generate_class_inventory.py", "port_manifest.yaml", "reports/agents/PGINV-003.md", "tests/oracle/test_class_inventory.py"], 3819))
    monkeypatch.setattr(
        runner,
        "diff_file_stats",
        lambda _worktree, _base: [
            {"path": "oracle/scripts/generate_class_inventory.py", "changed_lines": 339},
            {"path": "port_manifest.yaml", "changed_lines": 3047},
            {"path": "reports/agents/PGINV-003.md", "changed_lines": 30},
            {"path": "tests/oracle/test_class_inventory.py", "changed_lines": 403},
        ],
    )
    monkeypatch.setattr(runner, "add_labels", lambda _config, _number, values: labels.extend(values))
    monkeypatch.setattr(runner, "run_autoreview", lambda *_args, **_kwargs: (_ for _ in ()).throw(AssertionError("autoreview must not run when generated check fails")))
    monkeypatch.setattr(runner, "run", lambda args, **_kwargs: CommandResult(args=args, returncode=1, stdout="", stderr="stale manifest"))

    with pytest.raises(GateFailure, match="diff too large"):
        runner.review_issue(loaded, 10)

    assert config.github.human_review_label in labels


def test_review_failure_schedules_rework_without_human_label(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "github": {"rework_label": "ai:rework"},
            "validation": {"commands": []},
        },
        body="body",
    )
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    issue = Issue(5, "Pin reference", "body", ["tenant:cpp", "tag:bootstrap"], "url", "michel")
    created: list[dict[str, Any]] = []
    labels: list[str] = []
    comments: list[str] = []

    def fake_create(_config, _repo_root, **kwargs):
        created.append(kwargs)
        return f"task-{len(created)}"

    monkeypatch.setattr(runner, "view_issue", lambda _config, _issue_number: issue)
    monkeypatch.setattr(runner, "_kanban_create", fake_create)
    monkeypatch.setattr(runner, "add_labels", lambda _config, _number, values: labels.extend(values))
    monkeypatch.setattr(runner, "remove_labels", lambda _config, _number, _values: None)
    monkeypatch.setattr(runner, "comment_issue", lambda _config, _number, body: comments.append(body))

    result = _handle_phase_failure(
        loaded,
        issue_number=5,
        phase="review",
        reason="autoreview failed: actionable finding in scripts/bootstrap_reference",
    )

    assert result["action"] == "scheduled_rework"
    assert [item["assignee"] for item in created] == ["pi-worker", "pi-reviewer", "pi-release-manager"]
    assert [item["metadata"]["phase"] for item in created] == ["rework", "review", "release"]
    assert created[1]["parents"] == ["task-1"]
    assert created[2]["parents"] == ["task-2"]
    assert "ai:rework" in labels
    assert config.github.human_review_label not in labels
    assert comments == ["Rework 1/3 scheduled: review failed."]
    assert all(len(body) <= config.github_output.comment_max_chars for body in comments)


def test_scheduled_rework_supersedes_current_and_downstream_attempt_tasks(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "validation": {"commands": []},
        },
        body="body",
    )
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    issue = Issue(5, "Pin reference", "body", ["tag:bootstrap"], "url", "michel")
    created: list[dict[str, Any]] = []
    archived: list[list[str]] = []

    update_issue(
        tmp_path,
        5,
        lambda item: item.update(
            {
                "rework_attempts": 1,
                "rework_task_ids": {
                    "rework": "old-rework",
                    "review": "old-review",
                    "release": "old-release",
                },
            }
        ),
    )

    def fake_create(_config, _repo_root, **kwargs):
        created.append(kwargs)
        return f"new-task-{len(created)}"

    monkeypatch.setenv("HERMES_KANBAN_TASK", "old-review")
    monkeypatch.setattr(runner, "view_issue", lambda _config, _issue_number: issue)
    monkeypatch.setattr(runner, "_kanban_create", fake_create)
    monkeypatch.setattr(runner, "add_labels", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(runner, "remove_labels", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(runner, "comment_issue", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(runner, "review_issue", lambda *_args, **_kwargs: (_ for _ in ()).throw(GateFailure("autoreview failed: fix me")))
    monkeypatch.setattr(runner, "_archive_task_ids", lambda _config, task_ids: archived.append(task_ids))
    monkeypatch.setattr(runner, "_block_current_task", lambda *_args, **_kwargs: (_ for _ in ()).throw(AssertionError("scheduled rework should not leave stale blocked cards")))

    result = runner.run_issue_phase(loaded, issue_number=5, phase="review", complete_current_task=True)

    assert result["action"] == "scheduled_rework"
    assert result["superseded_tasks"] == ["old-review", "old-release"]
    assert archived == [["old-review", "old-release"]]
    assert [item["metadata"]["phase"] for item in created] == ["rework", "review", "release"]


def test_superseded_task_ids_keep_completed_rework_history():
    state = {
        "rework_task_ids": {
            "rework": "done-rework",
            "review": "current-review",
            "release": "downstream-release",
        }
    }

    assert runner._superseded_task_ids_for_phase_failure(state, "review", current_task_id="current-review") == [
        "current-review",
        "downstream-release",
    ]


def test_compact_pr_body_contains_only_summary_validation_and_closer():
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": "/tmp/workspaces"},
            "validation": {"commands": []},
        },
        body="body",
    )
    issue = Issue(12, "[AI] PGBOOT-012: Add concise GitHub output policy", "body", [], "url", "michel")
    state = {
        "changed_files": ["automation/pi_symphony/runner.py", "tests/test_pi_runner_command.py"],
        "changed_lines": 123,
    }
    validations = [
        {"command": "git diff --check", "returncode": 0, "output": ""},
        {"command": "python3 -m pytest -q", "returncode": 0, "output": "77 passed"},
    ]

    body = _pr_body(config, issue, state, validations)

    assert len(body) <= config.github_output.pr_body_max_chars
    assert body.startswith("## Summary\n")
    assert "Addresses #12" in body
    assert "## Validation" in body
    assert "Closes #12" in body
    assert "## Changed files" not in body
    assert "## Safety" not in body
    assert "automation/pi_symphony/runner.py" not in body
    assert "isolated git worktree" not in body


def test_intake_claim_does_not_comment_when_compact_claim_comments_disabled(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "validation": {"commands": []},
        },
        body="body",
    )
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    issue = Issue(7, "Small issue", "body", ["tenant:cpp"], "url", "michel")
    comments: list[str] = []

    monkeypatch.setattr(runner, "ensure_runtime_prereqs", lambda _config: [])
    monkeypatch.setattr(runner, "ensure_labels", lambda _config: [])
    monkeypatch.setattr(runner, "ensure_board", lambda _config, _repo_root: "pyqtgraph-to-cpp")
    monkeypatch.setattr(runner, "list_ready_issues", lambda _config, limit=None: [issue])
    monkeypatch.setattr(runner, "list_issue_items", lambda _config: [{"number": 7, "title": "Small issue", "body": "body", "state": "open", "labels": []}])
    monkeypatch.setattr(runner, "create_issue_task_graph", lambda _config, _repo_root, _issue: {"implement": "t1", "review": "t2", "release": "t3"})
    monkeypatch.setattr(runner, "add_labels", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(runner, "remove_labels", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(runner, "comment_issue", lambda _config, _number, body: comments.append(body))

    result = runner.intake(loaded)

    assert result["actions"] == [{"issue": 7, "action": "claimed", "tasks": {"implement": "t1", "review": "t2", "release": "t3"}}]
    assert comments == []


def test_implement_no_git_changes_schedules_rework_without_human_label(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "github": {"rework_label": "ai:rework"},
            "validation": {"commands": []},
        },
        body="body",
    )
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    issue = Issue(8, "Inventory", "body", ["tag:inventory"], "url", "michel")
    created: list[dict[str, Any]] = []
    labels: list[str] = []
    removed_labels: list[str] = []

    def fake_create(_config, _repo_root, **kwargs):
        created.append(kwargs)
        return f"task-{len(created)}"

    monkeypatch.setattr(runner, "view_issue", lambda _config, _issue_number: issue)
    monkeypatch.setattr(runner, "_kanban_create", fake_create)
    monkeypatch.setattr(runner, "add_labels", lambda _config, _number, values: labels.extend(values))
    monkeypatch.setattr(runner, "remove_labels", lambda _config, _number, values: removed_labels.extend(values))
    monkeypatch.setattr(runner, "comment_issue", lambda *_args, **_kwargs: None)

    result = _handle_phase_failure(
        loaded,
        issue_number=8,
        phase="implement",
        reason="Pi completed but left no git changes",
    )

    assert result["action"] == "scheduled_rework"
    assert [item["metadata"]["phase"] for item in created] == ["rework", "review", "release"]
    assert config.github.rework_label in labels
    assert config.github.blocked_label in removed_labels
    assert config.github.human_review_label in removed_labels
    assert config.github.human_review_label not in labels


def test_validation_failure_schedules_rework_without_human_label(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "github": {"rework_label": "ai:rework"},
            "validation": {"commands": []},
        },
        body="body",
    )
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    issue = Issue(8, "Inventory", "body", ["tag:inventory"], "url", "michel")
    labels: list[str] = []
    removed_labels: list[str] = []

    monkeypatch.setattr(runner, "view_issue", lambda _config, _issue_number: issue)
    monkeypatch.setattr(runner, "_kanban_create", lambda *_args, **_kwargs: "task")
    monkeypatch.setattr(runner, "add_labels", lambda _config, _number, values: labels.extend(values))
    monkeypatch.setattr(runner, "remove_labels", lambda _config, _number, values: removed_labels.extend(values))
    monkeypatch.setattr(runner, "comment_issue", lambda *_args, **_kwargs: None)

    result = _handle_phase_failure(
        loaded,
        issue_number=8,
        phase="review",
        reason="validation failed: python oracle/scripts/generate_source_inventory.py --check",
    )

    assert result["action"] == "scheduled_rework"
    assert config.github.rework_label in labels
    assert config.github.blocked_label in removed_labels
    assert config.github.human_review_label in removed_labels
    assert config.github.human_review_label not in labels


def test_permission_word_in_actionable_review_does_not_force_human_review(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "github": {"rework_label": "ai:rework"},
            "validation": {"commands": []},
        },
        body="body",
    )
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    issue = Issue(6, "Attribution", "body", ["tag:bootstrap"], "url", "michel")
    labels: list[str] = []

    monkeypatch.setattr(runner, "view_issue", lambda _config, _issue_number: issue)
    monkeypatch.setattr(runner, "_kanban_create", lambda *_args, **_kwargs: "task")
    monkeypatch.setattr(runner, "add_labels", lambda _config, _number, values: labels.extend(values))
    monkeypatch.setattr(runner, "remove_labels", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(runner, "comment_issue", lambda *_args, **_kwargs: None)

    result = _handle_phase_failure(
        loaded,
        issue_number=6,
        phase="review",
        reason="autoreview failed: permission text in attribution report needs code cleanup",
    )

    assert result["action"] == "scheduled_rework"
    assert config.github.rework_label in labels
    assert config.github.human_review_label not in labels


def test_ambiguous_requirement_in_actionable_review_does_not_force_human_review(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "github": {"rework_label": "ai:rework"},
            "validation": {"commands": []},
        },
        body="body",
    )
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    issue = Issue(6, "Attribution", "body", ["tag:bootstrap"], "url", "michel")
    labels: list[str] = []

    monkeypatch.setattr(runner, "view_issue", lambda _config, _issue_number: issue)
    monkeypatch.setattr(runner, "_kanban_create", lambda *_args, **_kwargs: "task")
    monkeypatch.setattr(runner, "add_labels", lambda _config, _number, values: labels.extend(values))
    monkeypatch.setattr(runner, "remove_labels", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(runner, "comment_issue", lambda *_args, **_kwargs: None)

    result = _handle_phase_failure(
        loaded,
        issue_number=6,
        phase="review",
        reason="autoreview failed: ambiguous requirement wording should be clarified in code comments",
    )

    assert result["action"] == "scheduled_rework"
    assert config.github.rework_label in labels
    assert config.github.human_review_label not in labels


def test_authentication_failed_labels_issue_for_human_review(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "github": {"rework_label": "ai:rework"},
            "validation": {"commands": []},
        },
        body="body",
    )
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    labels: list[str] = []

    monkeypatch.setattr(runner, "add_labels", lambda _config, _number, values: labels.extend(values))
    monkeypatch.setattr(runner, "remove_labels", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(runner, "comment_issue", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(runner, "_kanban_create", lambda *_args, **_kwargs: (_ for _ in ()).throw(AssertionError("auth failures must not schedule rework")))

    result = _handle_phase_failure(
        loaded,
        issue_number=3,
        phase="implement",
        reason="authentication failed: GitHub token expired",
    )

    assert result["action"] == "human_blocked"
    assert config.github.blocked_label in labels
    assert config.github.human_review_label in labels


def test_common_github_permission_failures_label_issue_for_human_review(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "github": {"rework_label": "ai:rework"},
            "validation": {"commands": []},
        },
        body="body",
    )
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    labels: list[str] = []

    monkeypatch.setattr(runner, "add_labels", lambda _config, _number, values: labels.extend(values))
    monkeypatch.setattr(runner, "remove_labels", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(runner, "comment_issue", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(runner, "_kanban_create", lambda *_args, **_kwargs: (_ for _ in ()).throw(AssertionError("permission failures must not schedule rework")))

    for reason in (
        "remote: Permission to michelreifenrath/pyqtgraph_to_cpp.git denied to bot",
        "Resource not accessible by integration",
        "requires credentials to push to GitHub",
        "fatal: could not read Username for 'https://github.com': terminal prompts disabled",
        "remote: Invalid username or password.",
    ):
        result = _handle_phase_failure(loaded, issue_number=3, phase="release", reason=reason)
        assert result["action"] == "human_blocked"

    assert config.github.blocked_label in labels
    assert config.github.human_review_label in labels


def test_pi_output_hard_blocker_evidence_is_preserved_for_failure_classification(tmp_path: Path):
    reason = runner._failure_reason_with_output_evidence(
        "Pi implementation failed with exit code 1",
        "remote: Permission to michelreifenrath/pyqtgraph_to_cpp.git denied to bot",
        tmp_path / "pi-output.md",
    )

    assert "permission to ... denied" in reason
    assert runner._requires_human_review(reason.lower())


def test_human_blocker_labels_issue_for_github_visibility(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "github": {"rework_label": "ai:rework"},
            "validation": {"commands": []},
        },
        body="body",
    )
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    labels: list[str] = []
    removed_labels: list[str] = []
    comments: list[str] = []

    monkeypatch.setattr(runner, "add_labels", lambda _config, _number, values: labels.extend(values))
    monkeypatch.setattr(runner, "remove_labels", lambda _config, _number, values: removed_labels.extend(values))
    monkeypatch.setattr(runner, "comment_issue", lambda _config, _number, body: comments.append(body))
    monkeypatch.setattr(runner, "_kanban_create", lambda *_args, **_kwargs: (_ for _ in ()).throw(AssertionError("human blockers must not schedule rework")))

    result = _handle_phase_failure(
        loaded,
        issue_number=3,
        phase="review",
        reason="missing required secret LINEAR_API_KEY",
    )

    assert result["action"] == "human_blocked"
    assert config.github.blocked_label in labels
    assert config.github.human_review_label in labels
    assert config.github.rework_label in removed_labels
    assert any("Human intervention required" in body for body in comments)


def test_design_decision_labels_issue_for_human_review(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "github": {"rework_label": "ai:rework"},
            "validation": {"commands": []},
        },
        body="body",
    )
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    labels: list[str] = []

    monkeypatch.setattr(runner, "add_labels", lambda _config, _number, values: labels.extend(values))
    monkeypatch.setattr(runner, "remove_labels", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(runner, "comment_issue", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(runner, "_kanban_create", lambda *_args, **_kwargs: (_ for _ in ()).throw(AssertionError("design decisions must not schedule rework")))

    result = _handle_phase_failure(
        loaded,
        issue_number=12,
        phase="review",
        reason="important architecture decision required: choose scene graph ownership model before implementation can continue",
    )

    assert result["action"] == "human_blocked"
    assert config.github.blocked_label in labels
    assert config.github.human_review_label in labels


def test_repeated_review_finding_blocks_before_retry_budget(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "agent": {"max_attempts": 10},
            "github": {"rework_label": "ai:rework"},
            "validation": {"commands": []},
        },
        body="body",
    )
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    previous_reason = """autoreview failed: autoreview target: branch
bundle: 32667 chars
[P2] Gate diff check ignores dirty working-tree changes
scripts/gate:54
The commit gate skips dirty edits.
"""
    current_reason = """autoreview failed: autoreview target: branch
bundle: 34566 chars
[P2] Gate diff check ignores dirty working-tree changes
scripts/gate:54
The same issue is still present with different volatile review metadata.
"""
    update_issue(
        tmp_path,
        4,
        lambda item: item.update({"rework_attempts": 4, "last_failure": previous_reason}),
    )
    labels: list[str] = []
    removed_labels: list[str] = []
    comments: list[str] = []

    monkeypatch.setattr(runner, "add_labels", lambda _config, _number, values: labels.extend(values))
    monkeypatch.setattr(runner, "remove_labels", lambda _config, _number, values: removed_labels.extend(values))
    monkeypatch.setattr(runner, "comment_issue", lambda _config, _number, body: comments.append(body))
    monkeypatch.setattr(runner, "_kanban_create", lambda *_args, **_kwargs: (_ for _ in ()).throw(AssertionError("repeated findings must not schedule more rework")))

    result = _handle_phase_failure(
        loaded,
        issue_number=4,
        phase="review",
        reason=current_reason,
    )

    assert result["action"] == "human_blocked"
    assert "repeated review finding" in result["reason"]
    assert config.github.blocked_label in labels
    assert config.github.human_review_label in labels
    assert config.github.rework_label in removed_labels
    assert any("repeated review finding" in body for body in comments)


def test_release_issue_refuses_uncommitted_changes_after_review(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "validation": {"commands": []},
        },
        body="body",
    )
    worktree = Path(config.workspace.root) / "issue-16"
    worktree.mkdir(parents=True)
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)

    monkeypatch.setattr(runner, "view_issue", lambda _config, _issue_number: Issue(16, "new files", "body", [], "url", "michel"))
    monkeypatch.setattr(
        runner,
        "get_issue",
        lambda _repo_root, _issue_number: {
            "status": "reviewed",
            "branch": "ai/issue-16-new-files",
            "autoreview": {"engine": "autoreview", "command": "autoreview --mode branch"},
            "autoreviewed_head": "reviewed-sha",
        },
    )
    monkeypatch.setattr(runner, "run_validations", lambda _config, _worktree, _issue_log_dir: [])
    monkeypatch.setattr(runner, "git_status_short", lambda _worktree: "?? generated.log")
    monkeypatch.setattr(runner, "run", lambda *_args, **_kwargs: (_ for _ in ()).throw(AssertionError("release must not push dirty worktree")))

    with pytest.raises(GateFailure, match="refusing to commit unreviewed changes"):
        runner.release_issue(loaded, 16)


def test_release_issue_refuses_head_that_was_not_reviewed(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "validation": {"commands": []},
        },
        body="body",
    )
    worktree = Path(config.workspace.root) / "issue-16"
    worktree.mkdir(parents=True)
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)

    monkeypatch.setattr(runner, "view_issue", lambda _config, _issue_number: Issue(16, "new files", "body", [], "url", "michel"))
    monkeypatch.setattr(
        runner,
        "get_issue",
        lambda _repo_root, _issue_number: {
            "status": "reviewed",
            "branch": "ai/issue-16-new-files",
            "reviewed_head": "reviewed-sha",
            "autoreview": {"engine": "autoreview", "command": "autoreview --mode branch"},
            "autoreviewed_head": "reviewed-sha",
        },
    )
    monkeypatch.setattr(runner, "run_validations", lambda _config, _worktree, _issue_log_dir: [])
    monkeypatch.setattr(runner, "git_status_short", lambda _worktree: "")

    def fake_run(args, **_kwargs):
        if args[:3] == ["git", "rev-parse", "HEAD"]:
            return CommandResult(args=args, returncode=0, stdout="different-sha\n", stderr="")
        raise AssertionError("release must not push an unreviewed head")

    monkeypatch.setattr(runner, "run", fake_run)

    with pytest.raises(GateFailure, match="does not match reviewed head"):
        runner.release_issue(loaded, 16)


def test_release_issue_uses_descriptive_pr_title(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": str(tmp_path / "workspaces")},
            "validation": {"commands": []},
        },
        body="body",
    )
    worktree = Path(config.workspace.root) / "issue-41"
    worktree.mkdir(parents=True)
    (tmp_path / ".hermes" / "pi-symphony" / "logs" / "issue-41").mkdir(parents=True)
    loaded = LoadedWorkflow(config=config, path=tmp_path / "WORKFLOW.md", repo_root=tmp_path)
    created: dict[str, Any] = {}

    monkeypatch.setattr(
        runner,
        "view_issue",
        lambda _config, _issue_number: Issue(
            41,
            "[AI] PGPLOT-004: Implement PlotCurveItem QPainter paint path",
            "body",
            ["tag:plot"],
            "url",
            "michel",
        ),
    )
    monkeypatch.setattr(
        runner,
        "get_issue",
        lambda _repo_root, _issue_number: {
            "status": "reviewed",
            "branch": "ai/issue-41-plotcurveitem-paint",
            "reviewed_head": "reviewed-sha",
            "changed_files": ["src/pyqtgraph/graphicsItems/PlotCurveItem.cpp"],
            "changed_lines": 25,
            "autoreview": {"engine": "autoreview", "command": "autoreview --mode branch"},
            "autoreviewed_head": "reviewed-sha",
        },
    )
    monkeypatch.setattr(runner, "run_validations", lambda _config, _worktree, _issue_log_dir: [])
    monkeypatch.setattr(runner, "git_status_short", lambda _worktree: "")

    def fake_run(args, **_kwargs):
        if args[:3] == ["git", "rev-parse", "HEAD"]:
            return CommandResult(args=args, returncode=0, stdout="reviewed-sha\n", stderr="")
        if args[:2] == ["git", "push"]:
            return CommandResult(args=args, returncode=0, stdout="", stderr="")
        raise AssertionError(f"unexpected command: {args}")

    def fake_create_pr(_config, *, branch: str, title: str, body: str):
        created.update({"branch": branch, "title": title, "body": body})
        return {"number": 99, "url": "https://example.invalid/pr/99"}

    monkeypatch.setattr(runner, "run", fake_run)
    monkeypatch.setattr(runner, "find_pr_for_branch", lambda _config, _branch: None)
    monkeypatch.setattr(runner, "create_pr", fake_create_pr)
    monkeypatch.setattr(runner, "add_labels", lambda *_args, **_kwargs: None)
    monkeypatch.setattr(runner, "remove_labels", lambda *_args, **_kwargs: None)

    runner.release_issue(loaded, 41)

    assert created["title"] == "feat(plotcurveitem): implement PlotCurveItem QPainter paint path"
