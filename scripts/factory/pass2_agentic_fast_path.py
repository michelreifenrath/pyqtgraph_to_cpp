#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


def _load_json(artifacts_dir: Path, name: str, default: Any = None) -> Any:
    path = artifacts_dir / name
    if not path.exists():
        return default
    return json.loads(path.read_text(encoding="utf-8"))


def _read_text(artifacts_dir: Path, name: str) -> str | None:
    path = artifacts_dir / name
    return path.read_text(encoding="utf-8").strip() if path.exists() else None


def decide(artifacts_dir: Path) -> dict[str, Any]:
    pass1 = _load_json(artifacts_dir, "pass1-summary.json", {}) or {}
    reviewers = [
        _load_json(artifacts_dir, "review-pass1-cpp-qt.json", {}) or {},
        _load_json(artifacts_dir, "review-pass1-oracle-visual.json", {}) or {},
        _load_json(artifacts_dir, "review-pass1-scope-governance.json", {}) or {},
    ]
    statuses = [
        _load_json(artifacts_dir, "readiness-status-pass1.json", {}) or {},
        _load_json(artifacts_dir, "scope-status-pass1.json", {}) or {},
        _load_json(artifacts_dir, "diff-check-status-pass1.json", {}) or {},
        _load_json(artifacts_dir, "local-gate-status-pass1.json", {}) or {},
        _load_json(artifacts_dir, "autoreview-status-pass1.json", {}) or {},
    ]
    visual = _load_json(artifacts_dir, "visual-oracle-pass1.json", {}) or {}
    fix_attempt = _load_json(artifacts_dir, "fix-attempt.json", {}) or {}

    reasons: list[str] = []
    if pass1.get("action") != "pass" or pass1.get("pass") is not True:
        reasons.append("pass-1 synthesis did not pass cleanly")
    if pass1.get("requires_human_review") is True or pass1.get("risky") is True or pass1.get("protected_files_changed") is True:
        reasons.append("pass-1 synthesis was risky/protected/human-review")
    if pass1.get("findings"):
        reasons.append("pass-1 synthesis reported findings")
    if any(status.get("ok") is not True for status in statuses):
        reasons.append("one or more pass-1 deterministic/autoreview gates failed")
    if visual.get("visual_ok") is not True:
        reasons.append("pass-1 visual/oracle gate did not pass")
    if any(
        reviewer.get("pass") is not True
        or reviewer.get("findings")
        or reviewer.get("requires_human_review") is True
        or reviewer.get("risky") is True
        or reviewer.get("protected_files_changed") is True
        for reviewer in reviewers
    ):
        reasons.append("one or more pass-1 reviewers were not clean")
    if fix_attempt.get("attempted") is True:
        reasons.append("a self-fix was attempted")

    pass1_head = _read_text(artifacts_dir, ".head-sha-pass1")
    current_head = _read_text(artifacts_dir, ".head-sha")
    pass1_base = _read_text(artifacts_dir, ".base-sha-pass1")
    current_base = _read_text(artifacts_dir, ".base-sha-pass2")
    if pass1_head != current_head:
        reasons.append("PR head changed after pass 1")
    if pass1_base != current_base:
        reasons.append("origin/main changed after pass 1")

    skip = not reasons
    return {
        "ok": True,
        "skip": skip,
        "reason": "pass-1 was clean and PR/base SHAs are unchanged" if skip else "; ".join(reasons),
        "pass1_head": pass1_head,
        "current_head": current_head,
        "pass1_base": pass1_base,
        "current_base": current_base,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Decide whether pass-2 agentic gates can use the clean pass-1 fast path.")
    parser.add_argument("--artifacts-dir", required=True)
    args = parser.parse_args(argv)
    artifacts_dir = Path(args.artifacts_dir)
    decision = decide(artifacts_dir)
    (artifacts_dir / "pass2-agentic-fast-path.json").write_text(json.dumps(decision, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(decision, indent=2, sort_keys=True))
    return 0 if decision["skip"] else 1


if __name__ == "__main__":
    sys.exit(main())
