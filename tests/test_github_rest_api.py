from __future__ import annotations

from automation.pi_symphony import github
from automation.pi_symphony.config import WorkflowConfig
from automation.pi_symphony.process import CommandResult


def _config() -> WorkflowConfig:
    return WorkflowConfig.from_mapping(
        {
            "tracker": {"kind": "github", "repo": "owner/repo"},
            "workspace": {"root": "/tmp/workspaces"},
            "pi": {"command": "pi"},
        }
    )


def test_view_issue_uses_rest_api_instead_of_graphql_issue_view(monkeypatch):
    calls: list[list[str]] = []

    def fake_run_json(args: list[str], **kwargs):
        calls.append(args)
        assert args[:4] == ["gh", "api", "-X", "GET"]
        assert args[4] == "repos/owner/repo/issues/42"
        return {
            "number": 42,
            "title": "[AI] Example",
            "body": "body",
            "html_url": "https://github.com/owner/repo/issues/42",
            "labels": [{"name": "ai:ready"}],
            "user": {"login": "michel"},
        }

    monkeypatch.setattr(github, "run_json", fake_run_json)

    issue = github.view_issue(_config(), 42)

    assert issue.number == 42
    assert issue.labels == ["ai:ready"]
    assert issue.url.endswith("/issues/42")
    assert calls


def test_add_remove_and_comment_issue_use_rest_api_not_issue_edit(monkeypatch):
    calls: list[list[str]] = []

    def fake_run(args, *, timeout=300, check=True, **kwargs):
        calls.append(args)
        assert args[:3] == ["gh", "api", "-X"]
        assert "issue" not in args[:3]
        return CommandResult(args=args, returncode=0, stdout="{}", stderr="")

    monkeypatch.setattr(github, "run", fake_run)

    config = _config()
    github.add_labels(config, 42, ["ai:rework", "tag:core"])
    github.remove_labels(config, 42, ["human-review"])
    github.comment_issue(config, 42, "hello")

    assert calls[0] == [
        "gh",
        "api",
        "-X",
        "POST",
        "repos/owner/repo/issues/42/labels",
        "-f",
        "labels[]=ai:rework",
        "-f",
        "labels[]=tag:core",
    ]
    assert calls[1] == ["gh", "api", "-X", "DELETE", "repos/owner/repo/issues/42/labels/human-review"]
    assert calls[2] == ["gh", "api", "-X", "POST", "repos/owner/repo/issues/42/comments", "-f", "body=hello"]


def test_remove_labels_is_idempotent_when_github_reports_missing_label(monkeypatch):
    def fake_run(args, *, timeout=300, check=True, **kwargs):
        return CommandResult(
            args=args,
            returncode=1,
            stdout='{"message":"Label does not exist","status":"404"}',
            stderr="gh: Label does not exist (HTTP 404)",
        )

    monkeypatch.setattr(github, "run", fake_run)

    github.remove_labels(_config(), 42, ["ai:blocked"])


def test_list_ready_issues_uses_rest_api_and_filters_active_labels(monkeypatch):
    def fake_run_json(args: list[str], **kwargs):
        assert args[:4] == ["gh", "api", "-X", "GET"]
        assert args[4] == "repos/owner/repo/issues"
        assert "-f" in args
        return [
            {
                "number": 1,
                "title": "[AI] Ready",
                "body": "body",
                "html_url": "https://github.com/owner/repo/issues/1",
                "labels": [{"name": "ai:ready"}, {"name": "tag:core"}],
                "user": {"login": "michel"},
            },
            {
                "number": 2,
                "title": "[AI] Already blocked",
                "body": "body",
                "html_url": "https://github.com/owner/repo/issues/2",
                "labels": [{"name": "ai:ready"}, {"name": "ai:blocked"}],
                "user": {"login": "michel"},
            },
        ]

    monkeypatch.setattr(github, "ensure_gh_authenticated", lambda: None)
    monkeypatch.setattr(github, "run_json", fake_run_json)

    issues = github.list_ready_issues(_config(), limit=10)

    assert [issue.number for issue in issues] == [1]


def test_list_ready_issues_prioritizes_product_validation_classes_before_deferable(monkeypatch):
    def fake_run_json(args: list[str], **kwargs):
        assert "per_page=100" in args
        return [
            {
                "number": 1,
                "title": "[P0.03] Dashboard infra",
                "body": "**Validation class:** manifest-infra\n",
                "html_url": "https://github.com/owner/repo/issues/1",
                "labels": [{"name": "ai:ready"}],
                "user": {"login": "michel"},
            },
            {
                "number": 2,
                "title": "[P2.05] Image bridge",
                "body": "**Validation class:** pixel-image\n",
                "html_url": "https://github.com/owner/repo/issues/2",
                "labels": [{"name": "ai:ready"}],
                "user": {"login": "michel"},
            },
            {
                "number": 3,
                "title": "[P2.10] Unknown class",
                "body": "body",
                "html_url": "https://github.com/owner/repo/issues/3",
                "labels": [{"name": "ai:ready"}],
                "user": {"login": "michel"},
            },
        ]

    monkeypatch.setattr(github, "ensure_gh_authenticated", lambda: None)
    monkeypatch.setattr(github, "run_json", fake_run_json)

    issues = github.list_ready_issues(_config(), limit=2)

    assert [issue.number for issue in issues] == [2, 3]
