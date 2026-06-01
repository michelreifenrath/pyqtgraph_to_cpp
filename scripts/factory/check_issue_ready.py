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

from scripts.factory.factory_common import (
    VALIDATION_CLASSES,
    GENERATED_REQUIRED_SECTIONS,
    LEGACY_REQUIRED_SECTIONS,
    PRODUCTION_FILE_CAP,
    TEST_ORACLE_FILE_CAP,
    TOTAL_FILE_CAP,
    VALIDATION_LEVELS,
    is_automation_issue,
    is_generated_local_issue,
    is_protected_file,
    issue_body_from_text,
    markdown_sections,
    metadata_fields,
    normalize_label_names,
    parse_blockers,
    parse_issue_ownership,
    parse_listish,
    parse_validation_levels,
    print_json,
    read_text_arg,
    section_lookup,
)


def dependencies_ready(text: str) -> tuple[bool, list[str]]:
    deps = parse_listish(text)
    if not deps:
        return False, []
    if len(deps) == 1 and deps[0].strip().lower() == "none":
        return True, deps
    for dep in deps:
        lowered = dep.lower()
        has_issue_reference = re.search(r"#\d+", dep)
        explicitly_resolved = re.search(r"\bresolved\b", lowered)
        explicitly_unresolved = re.search(r"\bunresolved\b|\bnot\s+resolved\b", lowered)
        if not has_issue_reference or not explicitly_resolved or explicitly_unresolved:
            return False, deps
    return True, deps


def scope_is_single_outcome(goal: str, scope: str) -> bool:
    goal_items = parse_listish(goal)
    if len(goal_items) > 1:
        return False
    combined = f"{goal}\n{scope}".lower()
    broad_markers = ["multiple outcomes", "several outcomes", "everything", "entire module", "broad refactor"]
    return not any(marker in combined for marker in broad_markers)


def _count_file_kinds(paths: list[str]) -> tuple[list[str], list[str], list[str]]:
    production_files = [
        path
        for path in paths
        if not path.startswith(("tests/", "test/", "oracle/", "reports/"))
        and path not in {"CMakeLists.txt", "port_manifest.yaml", "ownership.yaml"}
    ]
    test_oracle_files = [path for path in paths if path.startswith(("tests/", "test/", "oracle/", "reports/"))]
    example_files = [path for path in paths if path.startswith("examples/")]
    return production_files, test_oracle_files, example_files


def validate_generated_issue(body: str, metadata: dict[str, Any]) -> dict[str, Any]:
    errors: list[str] = []
    warnings: list[str] = []
    sections = markdown_sections(body)
    fields = metadata_fields(body)
    ownership = parse_issue_ownership(body, metadata)
    labels = normalize_label_names(metadata.get("labels", []))
    parsed: dict[str, Any] = {
        "schema": "generated-local-issue",
        "sections": sorted(sections),
        "issue_id": ownership["issue_id"],
        "validation_class": fields.get("Validation class", ""),
        "blocked_by": parse_blockers(fields.get("Blocked by", "")),
        "labels": labels,
        "owned_files": ownership["owned_files"],
        "repository_globs": ownership["repository_globs"],
        "common_adjuncts": ownership["common_adjuncts"],
        "selectors": ownership["selectors"],
    }

    missing = [name for name in GENERATED_REQUIRED_SECTIONS if not section_lookup(sections, name)]
    for name in missing:
        errors.append(f"missing required section: {name}")

    if not parsed["issue_id"]:
        errors.append("missing generated issue id (expected P<phase>.<number> in title or body)")
    if parsed["validation_class"] not in VALIDATION_CLASSES:
        errors.append(f"unknown validation class {parsed['validation_class']!r}")

    owned_text = section_lookup(sections, "Owned files")
    selectors = ownership["selectors"]
    selector_lines = {raw.strip().lstrip("-* ").split(":", 1)[0] for raw in owned_text.splitlines() if ":" in raw}
    for label in (
        "Manifest source selectors",
        "Manifest example selectors",
        "Repository path globs",
        "Common adjuncts",
        "Changed-file rule",
    ):
        if label not in selector_lines:
            errors.append(f"owned-file selector missing: {label}")
    if not owned_text:
        errors.append("owned files section is empty")
    if not (selectors["manifest_source"] or selectors["manifest_examples"] or selectors["repository_globs"] or ownership["owned_files"]):
        errors.append("owned files must include at least one manifest selector, example selector, repository glob, or expanded owned file")

    blocked_by = parsed["blocked_by"]
    has_ready_label = "ai:ready" in labels and not any(label in labels for label in ("ai:blocked", "ai:claimed", "ai:ignore", "ai:done"))
    if blocked_by and not has_ready_label:
        errors.append("blocked-by entries require resolved GitHub readiness labels before autonomous implementation")
    if metadata and not has_ready_label:
        errors.append("issue must carry ai:ready and not ai:blocked/ai:claimed/ai:ignore before autonomous implementation")

    production_files, test_oracle_files, example_files = _count_file_kinds(list(ownership["owned_files"]))
    if len(ownership["owned_files"]) > TOTAL_FILE_CAP:
        errors.append(f"expanded owned files exceed total cap of {TOTAL_FILE_CAP}")
    if len(production_files) > PRODUCTION_FILE_CAP:
        errors.append(f"production files exceed cap of {PRODUCTION_FILE_CAP}")
    if len(test_oracle_files) > TEST_ORACLE_FILE_CAP:
        errors.append(f"test/oracle files exceed cap of {TEST_ORACLE_FILE_CAP}")
    if len(example_files) > 1:
        errors.append("example files exceed cap of 1")

    protected = [
        path
        for path in list(ownership["owned_files"]) + list(ownership["repository_globs"])
        if is_protected_file(path)
    ]
    if protected and not is_automation_issue(body, metadata):
        errors.append("protected files require an explicit automation/governance marker")
    if protected:
        parsed["protected_files"] = protected

    ready = not errors
    return {"ready": ready, "errors": errors, "warnings": warnings, "parsed": parsed}


def validate_legacy_issue(body: str, metadata: dict[str, Any]) -> dict[str, Any]:
    errors: list[str] = []
    warnings: list[str] = []
    sections = markdown_sections(body)
    parsed: dict[str, Any] = {"schema": "legacy-factory-issue", "sections": sorted(sections)}

    missing = [name for name in LEGACY_REQUIRED_SECTIONS if not section_lookup(sections, name)]
    for name in missing:
        errors.append(f"missing required section: {name}")

    goal = section_lookup(sections, "Goal")
    reference = section_lookup(sections, "PyQtGraph reference")
    dependencies = section_lookup(sections, "Dependencies")
    owned_text = section_lookup(sections, "Owned files")
    scope = section_lookup(sections, "Scope")
    tdd = section_lookup(sections, "TDD plan")
    validation_level = section_lookup(sections, "Validation level")
    validation_commands = section_lookup(sections, "Validation commands")
    done = section_lookup(sections, "Done definition")

    ownership = parse_issue_ownership(body, metadata)
    owned_files = ownership["owned_files"]
    parsed.update(
        {
            "goal": goal,
            "pyqtgraph_reference": reference,
            "dependencies": parse_listish(dependencies),
            "owned_files": owned_files,
            "validation_levels": parse_validation_levels(validation_level),
            "validation_commands": parse_listish(validation_commands),
        }
    )

    if goal and not scope_is_single_outcome(goal, scope):
        errors.append("goal/scope must describe exactly one externally observable outcome")
    if not reference or reference.lower() in {"none", "n/a", "not_applicable"}:
        errors.append("PyQtGraph reference must name an upstream class, function, example, or behavior")

    deps_ok, _ = dependencies_ready(dependencies)
    if dependencies and not deps_ok:
        errors.append("dependencies must be 'none' or resolved issue references")

    if not owned_files:
        errors.append("owned files must list at least one file")
    if len(owned_files) > TOTAL_FILE_CAP:
        errors.append(f"owned files exceed total cap of {TOTAL_FILE_CAP}")
    production_files, test_oracle_files, example_files = _count_file_kinds(list(owned_files))
    if len(production_files) > PRODUCTION_FILE_CAP:
        errors.append(f"production files exceed cap of {PRODUCTION_FILE_CAP}")
    if len(test_oracle_files) > TEST_ORACLE_FILE_CAP:
        errors.append(f"test/oracle files exceed cap of {TEST_ORACLE_FILE_CAP}")
    if len(example_files) > 1:
        errors.append("example files exceed cap of 1")

    if tdd and not parse_listish(tdd):
        errors.append("TDD plan must include at least one concrete test/oracle step")
    if validation_commands and not parse_listish(validation_commands):
        errors.append("validation commands must include at least one command")
    if done and not parse_listish(done):
        errors.append("done definition must include concrete completion criteria")

    levels = parse_validation_levels(validation_level)
    for key in ("numeric", "visual", "interaction"):
        value = levels.get(key)
        if value is None:
            errors.append(f"validation level missing {key}: required/not_applicable")
        elif value not in VALIDATION_LEVELS:
            errors.append(f"validation level {key} must be required or not_applicable, got {value!r}")

    protected = [path for path in owned_files if is_protected_file(path)]
    if protected and not is_automation_issue(body, metadata):
        errors.append("protected files require an explicit automation/governance marker")
    if protected:
        parsed["protected_files"] = protected

    ready = not errors
    return {"ready": ready, "errors": errors, "warnings": warnings, "parsed": parsed}


def validate_issue(text: str) -> dict[str, Any]:
    body, metadata = issue_body_from_text(text)
    if is_generated_local_issue(body, metadata):
        return validate_generated_issue(body, metadata)
    return validate_legacy_issue(body, metadata)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Check whether a local factory issue is ready for autonomous implementation.")
    parser.add_argument("--issue-file", "-i", help="Issue markdown or JSON file. Defaults to stdin.")
    args = parser.parse_args(argv)
    try:
        result = validate_issue(read_text_arg(args.issue_file))
    except Exception as exc:  # fail closed for malformed local input
        result = {"ready": False, "errors": [str(exc)], "warnings": [], "parsed": {}}
    print_json(result)
    return 0 if result["ready"] else 1


if __name__ == "__main__":
    sys.exit(main())
