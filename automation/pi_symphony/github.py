from __future__ import annotations

from dataclasses import dataclass
from typing import Any
from urllib.parse import quote

from automation.pi_symphony.config import GithubConfig, WorkflowConfig
from automation.pi_symphony.process import run, run_json


@dataclass(frozen=True)
class Issue:
    number: int
    title: str
    body: str
    labels: list[str]
    url: str
    author: str


LABELS: dict[str, tuple[str, str]] = {
    "ai:ready": ("0e8a16", "AI automation may claim this issue."),
    "ai:claimed": ("1d76db", "AI automation has claimed this issue."),
    "ai:blocked": ("b60205", "AI automation needs human input or a missing prerequisite."),
    "ai:rework": ("d4c5f9", "AI automation is attempting bounded rework from review findings."),
    "ai:review": ("fbca04", "AI-created PR is ready for review."),
    "ai:merge-ready": ("0e8a16", "Current PR head passed mandatory autoreview and is ready for manual merge."),
    "ai:failed": ("d93f0b", "AI automation failed after retry budget or a hard gate."),
    "ai:done": ("5319e7", "AI automation completed this issue."),
    "ai:ignore": ("ededed", "Never automate this issue."),
    "human-review": ("5319e7", "Requires human review before merge or further automation."),
    "tag:bootstrap": ("bfdadc", "Bootstrap and automation setup."),
    "tag:inventory": ("bfdadc", "Inventory and manifest work."),
    "tag:oracle": ("bfdadc", "Validation oracle work."),
    "tag:core": ("bfdadc", "Core data/model helpers."),
    "tag:graphics": ("bfdadc", "Graphics scene/item/view work."),
    "tag:plot": ("bfdadc", "Plotting widget/item work."),
    "tag:examples": ("bfdadc", "Examples and smoke validation work."),
}


def ensure_gh_authenticated() -> None:
    run(["gh", "auth", "status"], timeout=60, check=True)


def ensure_labels(config: WorkflowConfig) -> list[str]:
    ensure_gh_authenticated()
    existing = {
        item["name"]
        for item in run_json(["gh", "label", "list", "--repo", config.tracker.repo, "--limit", "500", "--json", "name"])
    }
    required = _required_labels(config.github)
    created: list[str] = []
    for label in required:
        if label in existing:
            continue
        color, description = LABELS.get(label, ("ededed", "Pi Symphony automation label."))
        run(
            [
                "gh",
                "label",
                "create",
                label,
                "--repo",
                config.tracker.repo,
                "--color",
                color,
                "--description",
                description,
            ],
            timeout=60,
            check=True,
        )
        created.append(label)
    return created


def list_ready_issues(config: WorkflowConfig, *, limit: int | None = None) -> list[Issue]:
    ensure_gh_authenticated()
    labels = config.github
    raw = run_json(
        [
            "gh",
            "api",
            "-X",
            "GET",
            f"repos/{config.tracker.repo}/issues",
            "-f",
            "state=open",
            "-f",
            f"labels={labels.ready_label}",
            "-f",
            f"per_page={min(limit or config.agent.max_concurrent_issues, 100)}",
            "-f",
            "sort=created",
            "-f",
            "direction=asc",
        ]
    )
    candidates: list[Issue] = []
    for item in raw:
        if item.get("pull_request"):
            continue
        issue = _issue_from_rest_item(item)
        label_names = issue.labels
        if any(label in label_names for label in (labels.claimed_label, labels.blocked_label, labels.rework_label, labels.ignore_label, labels.done_label)):
            continue
        candidates.append(issue)
    if limit is not None:
        candidates = candidates[:limit]
    return candidates


def view_issue(config: WorkflowConfig, number: int) -> Issue:
    item = run_json(["gh", "api", "-X", "GET", f"repos/{config.tracker.repo}/issues/{number}"])
    return _issue_from_rest_item(item)


def _issue_from_rest_item(item: dict[str, Any]) -> Issue:
    label_names = [label["name"] if isinstance(label, dict) else str(label) for label in item.get("labels", [])]
    author = item.get("user") or item.get("author") or {}
    return Issue(
        number=int(item["number"]),
        title=str(item.get("title") or ""),
        body=str(item.get("body") or ""),
        labels=label_names,
        url=str(item.get("html_url") or item.get("url") or ""),
        author=str(author.get("login") if isinstance(author, dict) else author or ""),
    )


def add_labels(config: WorkflowConfig, number: int, labels: list[str]) -> None:
    if labels:
        cmd = ["gh", "api", "-X", "POST", f"repos/{config.tracker.repo}/issues/{number}/labels"]
        for label in labels:
            cmd.extend(["-f", f"labels[]={label}"])
        run(cmd, timeout=60)


def remove_labels(config: WorkflowConfig, number: int, labels: list[str]) -> None:
    for label in labels:
        result = run(
            ["gh", "api", "-X", "DELETE", f"repos/{config.tracker.repo}/issues/{number}/labels/{quote(label, safe='')}"],
            timeout=60,
            check=False,
        )
        if result.returncode != 0 and not _is_missing_label_error(result.combined_output):
            raise RuntimeError(result.combined_output)


def _is_missing_label_error(output: str) -> bool:
    normalized = output.lower()
    return (
        "not found" in normalized
        or "label does not exist" in normalized
        or "http 404" in normalized
        or '"status":"404"' in normalized
    )


def comment_issue(config: WorkflowConfig, number: int, body: str) -> None:
    run(["gh", "api", "-X", "POST", f"repos/{config.tracker.repo}/issues/{number}/comments", "-f", f"body={body}"], timeout=120)


def find_pr_for_branch(config: WorkflowConfig, branch: str) -> dict[str, Any] | None:
    raw = run_json(
        [
            "gh",
            "pr",
            "list",
            "--repo",
            config.tracker.repo,
            "--head",
            branch,
            "--state",
            "all",
            "--json",
            "number,state,url,title,isDraft,mergeStateStatus",
            "--limit",
            "20",
        ]
    )
    return raw[0] if raw else None


def create_pr(config: WorkflowConfig, *, branch: str, title: str, body: str) -> dict[str, Any]:
    run(
        [
            "gh",
            "pr",
            "create",
            "--repo",
            config.tracker.repo,
            "--head",
            branch,
            "--base",
            config.workspace.base_branch,
            "--title",
            title,
            "--body",
            body,
        ],
        timeout=180,
    )
    pr = find_pr_for_branch(config, branch)
    if pr is None:
        raise RuntimeError(f"PR for branch {branch!r} was created but could not be found")
    return pr


def list_ai_prs(config: WorkflowConfig) -> list[dict[str, Any]]:
    return run_json(
        [
            "gh",
            "pr",
            "list",
            "--repo",
            config.tracker.repo,
            "--state",
            "all",
            "--search",
            "head:ai/issue-",
            "--json",
            "number,state,url,title,headRefName,isDraft,mergeStateStatus,mergedAt",
            "--limit",
            "100",
        ]
    )


def _required_labels(labels: GithubConfig) -> list[str]:
    return [
        labels.ready_label,
        labels.claimed_label,
        labels.blocked_label,
        labels.rework_label,
        labels.review_label,
        labels.merge_ready_label,
        labels.failed_label,
        labels.done_label,
        labels.ignore_label,
        labels.human_review_label,
        "tag:bootstrap",
        "tag:inventory",
        "tag:oracle",
        "tag:core",
        "tag:graphics",
        "tag:plot",
        "tag:examples",
    ]
