from pathlib import Path

import pytest

from automation.pi_symphony import runner
from automation.pi_symphony.config import LoadedWorkflow, WorkflowConfig
from automation.pi_symphony.github import Issue
from automation.pi_symphony.process import CommandResult, run
from automation.pi_symphony.runner import GateFailure, _commit_worktree_changes_for_review, _pi_command
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
