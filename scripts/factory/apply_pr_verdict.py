#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from typing import Any

from scripts.factory.factory_common import print_json, read_text_arg


def decide(payload: dict[str, Any], allow_merge: bool = False) -> dict[str, Any]:
    errors: list[str] = []
    linked_issues = payload.get("linked_issues", [])
    if not isinstance(linked_issues, list) or len(linked_issues) != 1:
        errors.append("exactly one linked issue is required")

    required_true = ["readiness", "evidence", "scope", "tests", "holdout"]
    for key in required_true:
        if payload.get(key) is not True:
            errors.append(f"{key} must be true")
    if payload.get("visual_required") is True and payload.get("visual") is not True:
        errors.append("visual evidence is required but not passing")
    protected_files_changed = bool(payload.get("protected_files_changed"))
    risky = bool(payload.get("risky"))
    if protected_files_changed:
        errors.append("protected files changed")
    if risky:
        errors.append("risky PR requires human review")

    fix_attempts = int(payload.get("fix_attempts", 0) or 0)
    max_fix_attempts = int(payload.get("max_fix_attempts", 1) or 1)
    fixable = bool(payload.get("fixable", False))

    if not errors:
        decision = "merge"
        reason = "all gates passed"
    elif protected_files_changed or risky:
        decision = "human-review"
        reason = "; ".join(errors)
    elif fixable and fix_attempts < max_fix_attempts:
        decision = "fix"
        reason = "failed gates are marked fixable and retry budget remains"
    else:
        decision = "human-review"
        reason = "; ".join(errors)

    merge_command = None
    if decision == "merge" and allow_merge:
        pr_number = payload.get("pr_number", "${PR_NUMBER}")
        merge_command = f"gh pr merge {pr_number} --squash --delete-branch"

    return {
        "decision": decision,
        "dry_run": not allow_merge,
        "reason": reason,
        "errors": errors,
        "merge_command": merge_command,
        "would_merge": decision == "merge" and not allow_merge,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Compute a dry-run factory PR verdict.")
    parser.add_argument("--input", "-i", help="Verdict JSON file. Defaults to stdin.")
    parser.add_argument("--allow-merge", action="store_true", help="Only plans a gh merge command; this script never executes gh.")
    args = parser.parse_args(argv)
    try:
        payload = json.loads(read_text_arg(args.input))
        if not isinstance(payload, dict):
            raise ValueError("verdict input must be a JSON object")
        result = decide(payload, allow_merge=args.allow_merge)
    except Exception as exc:
        result = {"decision": "human-review", "dry_run": True, "reason": str(exc), "errors": [str(exc)], "merge_command": None, "would_merge": False}
    print_json(result)
    return 0 if result["decision"] == "merge" else 1


if __name__ == "__main__":
    sys.exit(main())
