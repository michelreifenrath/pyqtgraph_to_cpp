from pathlib import Path
from typing import Any

import pytest

from automation.pi_symphony import runner
from automation.pi_symphony.config import LoadedWorkflow, WorkflowConfig
from automation.pi_symphony.github import Issue
from automation.pi_symphony.process import CommandResult, run
from automation.pi_symphony.runner import GateFailure, _commit_worktree_changes_for_review, _handle_phase_failure, _pi_command
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

    commit = _commit_worktree_changes_for_review(repo, issue_number=16)

    assert commit is not None
    assert run(["git", "status", "--short"], cwd=repo, timeout=60).stdout.strip() == ""
    assert run(["git", "diff", "--name-only", "origin/main...HEAD"], cwd=repo, timeout=60).stdout.splitlines() == ["oracle/new_tool.py"]


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
    assert any("scheduled an automatic rework" in body for body in comments)


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
    monkeypatch.setattr(runner, "get_issue", lambda _repo_root, _issue_number: {"status": "reviewed", "branch": "ai/issue-16-new-files"})
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
    monkeypatch.setattr(runner, "get_issue", lambda _repo_root, _issue_number: {"status": "reviewed", "branch": "ai/issue-16-new-files", "reviewed_head": "reviewed-sha"})
    monkeypatch.setattr(runner, "run_validations", lambda _config, _worktree, _issue_log_dir: [])
    monkeypatch.setattr(runner, "git_status_short", lambda _worktree: "")

    def fake_run(args, **_kwargs):
        if args[:3] == ["git", "rev-parse", "HEAD"]:
            return CommandResult(args=args, returncode=0, stdout="different-sha\n", stderr="")
        raise AssertionError("release must not push an unreviewed head")

    monkeypatch.setattr(runner, "run", fake_run)

    with pytest.raises(GateFailure, match="does not match reviewed head"):
        runner.release_issue(loaded, 16)
