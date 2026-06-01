#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from scripts.factory.factory_common import (
    EXAMPLE_FILE_CAP,
    PRODUCTION_FILE_CAP,
    SHARED_INTEGRATION_CAP,
    TEST_ORACLE_FILE_CAP,
    TOTAL_FILE_CAP,
    classify_changed_file,
    is_automation_issue,
    is_protected_file,
    issue_body_from_text,
    normalize_path,
    parse_issue_ownership,
    print_json,
    read_text_arg,
)


def read_changed_files(args: argparse.Namespace) -> list[str]:
    files: list[str] = []
    files.extend(args.changed_file or [])
    if args.changed_files_file:
        files.extend(line.strip() for line in open(args.changed_files_file, encoding="utf-8") if line.strip())
    if not files and not sys.stdin.isatty():
        data = sys.stdin.read().strip()
        if data:
            if data.startswith("["):
                loaded = json.loads(data)
                files.extend(str(item) for item in loaded)
            else:
                files.extend(line.strip() for line in data.splitlines() if line.strip())
    return [normalize_path(path) for path in files if path.strip()]


def _count_file_kinds(changed_files: list[str]) -> tuple[list[str], list[str], list[str]]:
    production_files = [
        path
        for path in changed_files
        if not path.startswith(("tests/", "test/", "oracle/", "reports/"))
        and path not in {"CMakeLists.txt", "port_manifest.yaml", "ownership.yaml"}
    ]
    test_oracle_files = [path for path in changed_files if path.startswith(("tests/", "test/", "oracle/", "reports/"))]
    example_files = [path for path in changed_files if path.startswith("examples/")]
    return production_files, test_oracle_files, example_files


def validate_scope(issue_text: str, changed_files: list[str]) -> dict[str, object]:
    errors: list[str] = []
    warnings: list[str] = []
    body, metadata = issue_body_from_text(issue_text)
    ownership = parse_issue_ownership(body, metadata)
    owned_files = set(ownership["owned_files"])
    repository_globs = list(ownership["repository_globs"])
    common_adjuncts = list(ownership["common_adjuncts"])
    issue_id = str(ownership["issue_id"])
    owned_components = list(ownership["owned_components"])
    automation_issue = is_automation_issue(body, metadata)

    if not owned_files and not repository_globs and not common_adjuncts:
        errors.append("issue has no owned files or owned-file selectors")
    if not changed_files:
        errors.append("no changed files supplied")
    if len(changed_files) > TOTAL_FILE_CAP:
        errors.append(f"changed files exceed total cap of {TOTAL_FILE_CAP}")
    production_files, test_oracle_files, example_files = _count_file_kinds(changed_files)
    if len(production_files) > PRODUCTION_FILE_CAP:
        errors.append(f"production files exceed cap of {PRODUCTION_FILE_CAP}")
    if len(test_oracle_files) > TEST_ORACLE_FILE_CAP:
        errors.append(f"test/oracle files exceed cap of {TEST_ORACLE_FILE_CAP}")
    if len(example_files) > EXAMPLE_FILE_CAP:
        errors.append(f"example files exceed cap of {EXAMPLE_FILE_CAP}")

    classifications = {
        path: classify_changed_file(
            path,
            owned_files,
            repository_globs=repository_globs,
            common_adjuncts=common_adjuncts,
            issue_id=issue_id,
            owned_components=owned_components,
        )
        for path in changed_files
    }
    outside = [path for path, kind in classifications.items() if kind == "outside_scope"]
    shared = [path for path, kind in classifications.items() if kind == "shared_integration"]
    protected = [path for path in changed_files if is_protected_file(path)]

    if outside:
        errors.append("changed files outside owned files/shared allowlist: " + ", ".join(outside))
    if len(shared) > SHARED_INTEGRATION_CAP:
        errors.append(f"shared integration files exceed cap of {SHARED_INTEGRATION_CAP}")
    if protected and not automation_issue:
        errors.append("protected files changed without explicit automation/governance marker: " + ", ".join(protected))

    ok = not errors
    return {
        "ok": ok,
        "errors": errors,
        "warnings": warnings,
        "parsed": {
            "mode": ownership["mode"],
            "owned_files": sorted(owned_files),
            "repository_globs": repository_globs,
            "common_adjuncts": common_adjuncts,
            "issue_id": issue_id,
            "owned_components": owned_components,
            "changed_files": changed_files,
            "classifications": classifications,
            "protected_files": protected,
            "automation_issue": automation_issue,
        },
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Check a PR diff against issue-owned files.")
    parser.add_argument("--issue-file", "-i", required=True, help="Issue markdown or JSON file")
    parser.add_argument("--changed-file", action="append", help="Changed file path; repeatable")
    parser.add_argument("--changed-files-file", help="File containing changed paths, one per line")
    args = parser.parse_args(argv)
    try:
        result = validate_scope(read_text_arg(args.issue_file), read_changed_files(args))
    except Exception as exc:
        result = {"ok": False, "errors": [str(exc)], "warnings": [], "parsed": {}}
    print_json(result)
    return 0 if result["ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
