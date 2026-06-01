#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from typing import Any

from scripts.factory.check_issue_ready import validate_issue
from scripts.factory.factory_common import print_json, read_text_arg


def list_block(items: list[str]) -> str:
    return "\n".join(f"- {item}" for item in items) if items else "- none"


def selector_value(items: list[str]) -> str:
    return "; ".join(items) if items else "none"


def valid_issue_id(value: str) -> bool:
    return bool(re.fullmatch(r"P\d+\.\d+", value))


def render_issue(metadata: dict[str, Any]) -> dict[str, Any]:
    repo_globs = [str(item).strip() for item in metadata.get("owned_files", []) if str(item).strip()]
    source_selectors = [str(item).strip() for item in metadata.get("manifest_source_selectors", []) if str(item).strip()]
    example_selectors = [str(item).strip() for item in metadata.get("manifest_example_selectors", []) if str(item).strip()]
    common_adjuncts = [str(item).strip() for item in metadata.get("common_adjuncts", ["focused-tests"]) if str(item).strip()]
    command = str(metadata.get("command", "not provided"))
    summary = str(metadata.get("summary", "Regression detected by comprehensive factory test"))
    expected = str(metadata.get("expected", "Existing passing behavior"))
    actual = str(metadata.get("actual", "Command failed"))
    artifacts = [str(item) for item in metadata.get("artifacts", [])]
    suspected = str(metadata.get("suspected_subsystem", "unknown"))
    issue_id = str(metadata.get("issue_id", "")).strip()
    validation_class = str(metadata.get("validation_class", "script-infra"))
    risk = str(metadata.get("risk", "medium"))

    title = f"Regression: {summary}"
    heading = f"# {issue_id}: {summary}" if valid_issue_id(issue_id) else f"# Regression: {summary}"
    body = f"""<!-- generated-local-issue -->
{heading}

**Status:** todo
**Type:** AFK
**Validation class:** {validation_class}
**Risk:** {risk}
**Blocked by:** None
**Validation authority:** local scripts/CMake/tests only; do not add GitHub Actions dependency.

## Goal
Restore the externally observable behavior covered by the failing regression command: `{command}`.

## Current evidence
- Summary: {summary}
- Expected: {expected}
- Actual: {actual}
- Suspected subsystem: {suspected}
- Artifacts:
{list_block(artifacts)}

## Scope
- Reproduce only the regression reported by this evidence.
- Add or update the smallest local proof that fails before the fix.
- Implement the minimal C++/Qt or local-validation fix; do not refactor unrelated code.

## Owned files
- Manifest source selectors: {selector_value(source_selectors)}
- Manifest example selectors: {selector_value(example_selectors)}
- Repository path globs: {selector_value(repo_globs)}
- Common adjuncts: {selector_value(common_adjuncts)}
- Changed-file rule: every modified path must match these selectors or the named adjunct set; otherwise update this issue before implementation.

## Required local proof
- Task-specific outcome: the regression command passes and the new focused regression proof fails before the fix and passes after it.
- Proof command/artifact: `{command}`
- Evidence detail: record exact command(s), exit code(s), and artifact path(s) under `reports/issues/{issue_id or 'regression'}/`.

## TDD plan
- Reproduce the failing command locally.
- Add or update the smallest regression test/oracle that fails before the fix.
- Implement the minimal fix and rerun validation.

## Validation commands
- `{command}`
- `git diff --check`
- `git diff --name-only origin/main...HEAD`

## Acceptance criteria
- [ ] Regression command passes.
- [ ] Added or updated focused regression coverage passes.
- [ ] Changed files stay within `## Owned files` or this issue is updated before implementation.
- [ ] Completion report records exact local command(s), exit code(s), and artifact path(s).

## Done definition
- [ ] Focused proof for this issue passes and records exact command(s), exit code(s), and artifact path(s).
- [ ] Changed-file ownership check passes, or this issue was updated before implementation to include any extra path.
- [ ] Required manifest/dashboard updates are complete, or the completion report states why they are not applicable.

## Scope boundaries
- Edit only files allowed by `## Owned files`; expand scope by updating this issue before implementation, not after.
- Preserve PyQtGraph names, hierarchy, and example names unless the parity contract approves an idiomatic C++ equivalent.
- Do not create or require GitHub Actions; all proof must be runnable locally.
"""
    readiness_input = json.dumps({"body": body, "title": title, "labels": ["ai:ready"]})
    readiness = validate_issue(readiness_input)
    ready = bool(readiness["ready"] and valid_issue_id(issue_id) and (repo_globs or source_selectors or example_selectors))
    labels = ["factory:from-regression"]
    if ready:
        labels.extend(["ai:ready", "factory:ready-checked"])
    else:
        labels.extend(["ai:blocked", "human-review"])
    return {
        "title": title,
        "body": body,
        "labels": labels,
        "ready": ready,
        "readiness_errors": [] if ready else readiness["errors"] or ["regression metadata must include issue_id and owned selectors before ai:ready"],
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Render a local regression issue payload without calling GitHub.")
    parser.add_argument("--input", "-i", help="Regression metadata JSON file. Defaults to stdin.")
    args = parser.parse_args(argv)
    try:
        payload = json.loads(read_text_arg(args.input))
        if not isinstance(payload, dict):
            raise ValueError("regression metadata must be a JSON object")
        result = render_issue(payload)
    except Exception as exc:
        result = {"title": "Regression: invalid metadata", "body": "", "labels": ["ai:blocked", "human-review"], "ready": False, "errors": [str(exc)]}
    print_json(result)
    return 0


if __name__ == "__main__":
    sys.exit(main())
