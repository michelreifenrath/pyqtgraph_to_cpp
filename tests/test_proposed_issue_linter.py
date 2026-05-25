from __future__ import annotations

import json
from pathlib import Path

from automation.pi_symphony.proposed_issue_linter import (
    github_label_updates,
    lint_issue_map,
    lint_issues,
    load_issues,
    parse_issue_text,
)


ISSUE_TEMPLATE = """<!-- generated-local-issue -->
# {issue_id}: Example issue

**Status:** todo  
**Type:** AFK  
**Validation class:** {validation_class}  
**Risk:** medium  
**Blocked by:** {blocked_by}  
**Validation authority:** local scripts/CMake/tests only; do not add GitHub Actions dependency.

## Goal
Do the thing.

## Current evidence
No sufficient implementation found in the current checkout. Keep open.

## Scope
- Implement only this issue.

## Owned files
- Manifest source selectors: pyqtgraph/example.py
- Manifest example selectors: none
- Repository path globs: none
- Common adjuncts: focused-tests
- Changed-file rule: every modified path must match these selectors or the named adjunct set; otherwise update this issue before implementation.

## Required local proof
- Task-specific outcome: focused proof passes.

## TDD plan
- Add the smallest focused failing test first when behavior changes.

## Validation commands
- `ctest --preset dev -L {issue_id}`

## Acceptance criteria
- [ ] Focused proof passes.

## Done definition
- [ ] Completion report records commands, exit codes, and artifacts.

## Scope boundaries
- Edit only files allowed by `## Owned files`.
"""


def write_issue(root: Path, issue_id: str, *, blocked_by: str = "None", validation_class: str = "api-runtime") -> Path:
    path = root / f"{issue_id}-example.md"
    path.write_text(
        ISSUE_TEMPLATE.format(issue_id=issue_id, blocked_by=blocked_by, validation_class=validation_class)
    )
    return path


def test_linter_accepts_normalized_explicit_issue(tmp_path: Path):
    write_issue(tmp_path, "P0.01")
    write_issue(tmp_path, "P0.02", blocked_by="P0.01")

    assert lint_issues(load_issues(tmp_path)) == []


def test_linter_rejects_non_explicit_dependency_and_selector_prose(tmp_path: Path):
    path = write_issue(tmp_path, "P4.15", blocked_by="P4.01-P4.14")
    text = path.read_text().replace(
        "- Manifest source selectors: pyqtgraph/example.py",
        "- Manifest source selectors: pyqtgraph/example.py plus prose",
    )
    path.write_text(text)

    messages = [message.message for message in lint_issues(load_issues(tmp_path))]

    assert "blocked-by entry is not an explicit issue id: 'P4.01-P4.14'" in messages
    assert any("owned-file selector contains unparseable prose" in message for message in messages)


def test_linter_rejects_stale_issue_map_blocker_metadata(tmp_path: Path):
    write_issue(tmp_path, "P4.01")
    write_issue(tmp_path, "P4.02")
    write_issue(tmp_path, "P4.15", blocked_by="P4.01, P4.02")
    issue_map = tmp_path / "github-issue-map.json"
    issue_map.write_text(
        json.dumps(
            {
                "created": [
                    {"id": "P4.01", "number": 101, "blocked": "None"},
                    {"id": "P4.02", "number": 102, "blocked": "None"},
                    {"id": "P4.15", "number": 115, "blocked": "P4.01-P4.02"},
                ]
            }
        )
    )

    messages = [message.message for message in lint_issue_map(issue_map, load_issues(tmp_path))]

    assert "issue-map blocked entry for P4.15 is not an explicit issue id: 'P4.01-P4.02'" in messages
    assert "issue-map blockers for P4.15 do not match local issue: P4.01-P4.02 != P4.01, P4.02" in messages


def test_github_label_updates_can_use_github_issue_numbers_without_local_map(tmp_path: Path):
    issues = [
        parse_issue_text(Path("github-issue-95.md"), ISSUE_TEMPLATE.format(issue_id="P0.01", blocked_by="None", validation_class="api-runtime"), number=95),
        parse_issue_text(Path("github-issue-96.md"), ISSUE_TEMPLATE.format(issue_id="P0.02", blocked_by="P0.01", validation_class="api-runtime"), number=96),
    ]

    updates = github_label_updates(tmp_path / "missing-map.json", issues)

    assert updates == [
        {"id": "P0.01", "number": 95, "blocked": False, "remove": ["ai:blocked"], "add": ["ai:ready"]},
        {"id": "P0.02", "number": 96, "blocked": True, "remove": ["ai:ready"], "add": ["ai:blocked"]},
    ]


def test_github_label_updates_block_only_dependency_free_issues(tmp_path: Path):
    write_issue(tmp_path, "P0.01")
    write_issue(tmp_path, "P0.02", blocked_by="P0.01")
    issue_map = tmp_path / "github-issue-map.json"
    issue_map.write_text(
        json.dumps(
            {
                "created": [
                    {"id": "P0.01", "number": 95},
                    {"id": "P0.02", "number": 96},
                ]
            }
        )
    )

    updates = github_label_updates(issue_map, load_issues(tmp_path))

    assert updates == [
        {"id": "P0.01", "number": 95, "blocked": False, "remove": ["ai:blocked"], "add": ["ai:ready"]},
        {"id": "P0.02", "number": 96, "blocked": True, "remove": ["ai:ready"], "add": ["ai:blocked"]},
    ]
