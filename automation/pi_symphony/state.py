from __future__ import annotations

import fcntl
import json
from collections.abc import Callable
from contextlib import contextmanager
from pathlib import Path
from typing import Any

STATE_VERSION = 1


def state_dir(repo_root: Path) -> Path:
    return repo_root / ".hermes" / "pi-symphony" / "state"


def logs_dir(repo_root: Path, issue_number: int | None = None) -> Path:
    base = repo_root / ".hermes" / "pi-symphony" / "logs"
    if issue_number is not None:
        return base / f"issue-{issue_number}"
    return base


def state_path(repo_root: Path) -> Path:
    return state_dir(repo_root) / "state.json"


def default_state() -> dict[str, Any]:
    return {"version": STATE_VERSION, "issues": {}}


@contextmanager
def locked_state(repo_root: Path):
    directory = state_dir(repo_root)
    directory.mkdir(parents=True, exist_ok=True)
    lock_path = directory / "state.lock"
    with lock_path.open("w", encoding="utf-8") as lock_file:
        fcntl.flock(lock_file.fileno(), fcntl.LOCK_EX)
        state = load_state(repo_root)
        yield state
        save_state(repo_root, state)
        fcntl.flock(lock_file.fileno(), fcntl.LOCK_UN)


def load_state(repo_root: Path) -> dict[str, Any]:
    path = state_path(repo_root)
    if not path.exists():
        return default_state()
    with path.open("r", encoding="utf-8") as fh:
        data = json.load(fh)
    if not isinstance(data, dict):
        return default_state()
    data.setdefault("version", STATE_VERSION)
    data.setdefault("issues", {})
    return data


def save_state(repo_root: Path, state: dict[str, Any]) -> None:
    path = state_path(repo_root)
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(".json.tmp")
    with tmp.open("w", encoding="utf-8") as fh:
        json.dump(state, fh, indent=2, sort_keys=True)
        fh.write("\n")
    tmp.replace(path)


def get_issue(repo_root: Path, issue_number: int) -> dict[str, Any]:
    return load_state(repo_root).get("issues", {}).get(str(issue_number), {})


def update_issue(repo_root: Path, issue_number: int, updater: Callable[[dict[str, Any]], None]) -> dict[str, Any]:
    with locked_state(repo_root) as state:
        issues = state.setdefault("issues", {})
        issue = issues.setdefault(str(issue_number), {"attempts": 0})
        updater(issue)
        return dict(issue)
