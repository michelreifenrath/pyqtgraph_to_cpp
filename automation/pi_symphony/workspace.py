from __future__ import annotations

import re
from pathlib import Path

from automation.pi_symphony.config import WorkflowConfig
from automation.pi_symphony.process import run


def issue_slug(title: str, *, max_len: int = 42) -> str:
    slug = title.strip().lower()
    slug = re.sub(r"[^a-z0-9]+", "-", slug).strip("-")
    return (slug[:max_len].strip("-") or "work")


def branch_name(issue_number: int, title: str) -> str:
    return f"ai/issue-{issue_number}-{issue_slug(title)}"


def worktree_path(config: WorkflowConfig, issue_number: int) -> Path:
    return Path(config.workspace.root).expanduser().resolve() / f"issue-{issue_number}"


def ensure_worktree(config: WorkflowConfig, repo_root: Path, issue_number: int, title: str) -> tuple[Path, str]:
    path = worktree_path(config, issue_number)
    branch = branch_name(issue_number, title)
    path.parent.mkdir(parents=True, exist_ok=True)

    run(["git", "fetch", "origin", config.workspace.base_branch], cwd=repo_root, timeout=300, check=False)
    if path.exists() and (path / ".git").exists():
        return path, branch

    base_ref = f"origin/{config.workspace.base_branch}"
    result = run(["git", "rev-parse", "--verify", base_ref], cwd=repo_root, timeout=60, check=False)
    if result.returncode != 0:
        base_ref = config.workspace.base_branch
    run(["git", "worktree", "add", "-B", branch, str(path), base_ref], cwd=repo_root, timeout=300)
    return path, branch


def git_status_short(path: Path) -> str:
    return run(["git", "status", "--short"], cwd=path, timeout=60).stdout.strip()


def diff_file_stats(path: Path, base: str = "origin/main") -> list[dict[str, int | str]]:
    diff_ref = f"{base}...HEAD"
    numstat = run(["git", "diff", "--numstat", diff_ref, "--"], cwd=path, timeout=120, check=False).stdout
    stats: list[dict[str, int | str]] = []
    for line in numstat.splitlines():
        parts = line.split("\t")
        if len(parts) < 3:
            continue
        added = int(parts[0]) if parts[0].isdigit() else 0
        deleted = int(parts[1]) if parts[1].isdigit() else 0
        stats.append({"path": parts[-1], "changed_lines": added + deleted})
    return stats


def diff_stats(path: Path, base: str = "origin/main") -> tuple[list[str], int]:
    stats = diff_file_stats(path, base)
    files = [str(item["path"]) for item in stats]
    changed_lines = sum(int(item["changed_lines"]) for item in stats)
    return files, changed_lines
