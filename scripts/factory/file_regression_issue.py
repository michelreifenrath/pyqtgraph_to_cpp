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

from scripts.factory.check_issue_ready import validate_issue
from scripts.factory.factory_common import print_json, read_text_arg


def list_block(items: list[str]) -> str:
    return "\n".join(f"- {item}" for item in items) if items else "- not_applicable"


def render_issue(metadata: dict[str, Any]) -> dict[str, Any]:
    owned_files = [str(item) for item in metadata.get("owned_files", []) if str(item).strip()]
    command = str(metadata.get("command", "not provided"))
    summary = str(metadata.get("summary", "Regression detected by comprehensive factory test"))
    expected = str(metadata.get("expected", "Existing passing behavior"))
    actual = str(metadata.get("actual", "Command failed"))
    artifacts = [str(item) for item in metadata.get("artifacts", [])]
    suspected = str(metadata.get("suspected_subsystem", "unknown"))

    body = f"""## Goal
Restore the externally observable behavior covered by the failing regression command: `{command}`.

## PyQtGraph reference
{metadata.get('pyqtgraph_reference', 'not_applicable')}

## Dependencies
none

## Owned files
{list_block(owned_files)}

## Scope
Fix the regression reported by the factory evidence only; do not refactor unrelated code.

## TDD plan
- Reproduce the failing command locally.
- Add or update the smallest regression test/oracle that fails before the fix.
- Implement the minimal fix and rerun validation.

## Validation level
numeric: required
visual: {metadata.get('visual_level', 'not_applicable')}
interaction: not_applicable

## Validation commands
- {command}

## Done definition
- Regression command passes.
- Added or updated regression coverage passes.
- Changed files stay within the owned files.

## Regression evidence
- Summary: {summary}
- Expected: {expected}
- Actual: {actual}
- Suspected subsystem: {suspected}
- Artifacts:
{list_block(artifacts)}
"""
    readiness = validate_issue(body)
    ready = bool(readiness["ready"])
    labels = ["factory:from-regression"]
    if ready:
        labels.extend(["ai:ready", "factory:ready-checked"])
    else:
        labels.extend(["ai:blocked", "human-review"])
    return {
        "title": f"Regression: {summary}",
        "body": body,
        "labels": labels,
        "ready": ready,
        "readiness_errors": readiness["errors"],
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
