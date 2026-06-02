#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from typing import Any

from scripts.factory.factory_common import print_json, read_text_arg


def _accepted_gpt_visual_review(payload: dict[str, Any]) -> bool:
    if payload.get("gpt_visual_review") is True:
        return True
    review = payload.get("gpt_visual_review")
    if isinstance(review, dict):
        verdict = str(review.get("verdict", "")).lower()
        recommendation = str(review.get("recommendation", "")).lower()
        return verdict in {"pass", "passed", "ok"} and recommendation in {"merge_ok", "merge-ok", "merge ok"}
    review = payload.get("semantic_visual_review")
    if isinstance(review, dict):
        verdict = str(review.get("verdict", "")).lower()
        recommendation = str(review.get("recommendation", "")).lower()
        return verdict in {"pass", "passed", "ok"} and recommendation in {"merge_ok", "merge-ok", "merge ok"}
    return payload.get("gpt_visual_review_accepted") is True


def _has_gpt_visual_review(payload: dict[str, Any]) -> bool:
    return any(
        key in payload and payload.get(key) not in {None, False, ""}
        for key in ("gpt_visual_review", "semantic_visual_review", "gpt_visual_review_accepted")
    )


def _any_true(payload: dict[str, Any], *keys: str) -> bool:
    return any(payload.get(key) is True for key in keys)


def merge_command(payload: dict[str, Any]) -> list[str]:
    pr_number = payload.get("pr_number")
    head_sha = payload.get("head_sha")
    if not pr_number:
        raise ValueError("pr_number is required for merge")
    if not isinstance(head_sha, str) or not head_sha.strip():
        raise ValueError("head_sha is required for guarded merge")
    return ["gh", "pr", "merge", str(pr_number), "--squash", "--delete-branch", "--match-head-commit", head_sha.strip()]


def decide(payload: dict[str, Any], allow_merge: bool = False) -> dict[str, Any]:
    errors: list[str] = []
    automatable_errors: list[str] = []
    human_review_errors: list[str] = []

    def add_automatable(message: str) -> None:
        errors.append(message)
        automatable_errors.append(message)

    def add_human_review(message: str) -> None:
        errors.append(message)
        human_review_errors.append(message)

    linked_issues = payload.get("linked_issues", [])
    if not isinstance(linked_issues, list) or len(linked_issues) != 1:
        add_human_review("exactly one linked issue is required")

    required_true = ["readiness", "evidence", "scope", "tests", "holdout", "autoreview", "diff_check"]
    for key in required_true:
        if payload.get(key) is not True:
            add_automatable(f"{key} must be true")

    if payload.get("auto_merge_enabled") is not True:
        add_human_review("auto_merge_enabled must be true")

    state = str(payload.get("pr_state", payload.get("state", ""))).upper()
    if state != "OPEN":
        add_human_review("PR must be open")
    if payload.get("is_draft", payload.get("isDraft")) is not False:
        add_human_review("PR must not be draft")
    if payload.get("base_ref", payload.get("baseRefName")) != "main":
        add_human_review("PR base branch must be main")
    head_ref = payload.get("head_ref", payload.get("headRefName"))
    if not isinstance(head_ref, str) or not head_ref.strip():
        add_human_review("PR head branch is required")
    elif head_ref == "main":
        add_human_review("PR head branch must not be main")
    if not isinstance(payload.get("head_sha"), str) or not payload.get("head_sha", "").strip():
        add_human_review("head_sha is required")

    if payload.get("oracle_required") is True and not _any_true(payload, "oracle", "oracle_evidence", "oracle_passed"):
        add_automatable("oracle evidence is required but not passing")
    if payload.get("numeric_required") is True and not _any_true(payload, "numeric", "numeric_evidence", "numeric_passed"):
        add_automatable("numeric evidence is required but not passing")

    if payload.get("visual_required") is True:
        if payload.get("visual") is not True:
            add_automatable("visual evidence is required but not passing")
        if not _accepted_gpt_visual_review(payload):
            if _has_gpt_visual_review(payload):
                add_human_review("GPT semantic visual review rejected or disagrees")
            else:
                add_automatable("GPT semantic visual review is required but missing")

    protected_files_changed = bool(payload.get("protected_files_changed"))
    risky = bool(payload.get("risky"))
    if protected_files_changed:
        add_human_review("protected files changed")
    if risky:
        add_human_review("risky PR requires human review")

    fix_attempts = int(payload.get("fix_attempts", 0) or 0)
    max_fix_attempts = int(payload.get("max_fix_attempts", 1) or 1)
    retry_budget_available = fix_attempts < max_fix_attempts
    # Ordinary gate/evidence failures are actionable by the factory unless the
    # holdout explicitly marks them non-fixable. Human review is reserved for
    # policy/safety/metadata blockers or exhausted retry budget.
    fixable = payload.get("fixable") is not False

    command: list[str] | None = None
    if not errors:
        decision = "merge"
        reason = "all governed auto-merge gates passed"
        command = merge_command(payload)
    elif human_review_errors:
        decision = "human-review"
        reason = "; ".join(human_review_errors)
    elif fixable and retry_budget_available:
        decision = "fix"
        reason = "automatable gate/evidence failures remain and retry budget is available"
    else:
        decision = "human-review"
        reason = "retry budget exhausted or holdout marked failures non-fixable: " + "; ".join(errors)

    would_merge = decision == "merge" and allow_merge
    return {
        "decision": decision,
        "dry_run": not allow_merge,
        "reason": reason,
        "errors": errors,
        "automatable_errors": automatable_errors,
        "human_review_errors": human_review_errors,
        "merge_command": " ".join(command) if command else None,
        "merge_argv": command,
        "ai_label": "ai:merge-ready" if decision == "merge" and not allow_merge else None,
        "would_label_merge_ready": decision == "merge" and not allow_merge,
        "would_merge": would_merge,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Compute or execute a governed factory PR verdict.")
    parser.add_argument("--input", "-i", help="Verdict JSON file. Defaults to stdin.")
    parser.add_argument("--allow-merge", action="store_true", help="Execute guarded gh pr merge when all governed gates pass.")
    args = parser.parse_args(argv)
    try:
        payload = json.loads(read_text_arg(args.input))
        if not isinstance(payload, dict):
            raise ValueError("verdict input must be a JSON object")
        result = decide(payload, allow_merge=args.allow_merge)
        if result["would_merge"]:
            completed = subprocess.run(result["merge_argv"], check=False, text=True, capture_output=True)
            result["merge_returncode"] = completed.returncode
            result["merge_stdout"] = completed.stdout
            result["merge_stderr"] = completed.stderr
            if completed.returncode != 0:
                result["decision"] = "human-review"
                result["reason"] = "guarded gh pr merge failed"
                result["errors"] = ["guarded gh pr merge failed"]
                result["automatable_errors"] = []
                result["human_review_errors"] = ["guarded gh pr merge failed"]
    except Exception as exc:
        result = {
            "decision": "human-review",
            "dry_run": True,
            "reason": str(exc),
            "errors": [str(exc)],
            "automatable_errors": [],
            "human_review_errors": [str(exc)],
            "merge_command": None,
            "merge_argv": None,
            "ai_label": None,
            "would_label_merge_ready": False,
            "would_merge": False,
        }
    print_json(result)
    return 0 if result["decision"] == "merge" else 1


if __name__ == "__main__":
    sys.exit(main())
