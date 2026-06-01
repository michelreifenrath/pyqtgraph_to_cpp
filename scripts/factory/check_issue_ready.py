#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from typing import Any

from scripts.factory.factory_common import (
    REQUIRED_SECTIONS,
    EXAMPLE_FILE_CAP,
    PRODUCTION_FILE_CAP,
    TEST_ORACLE_FILE_CAP,
    TOTAL_FILE_CAP,
    VALIDATION_LEVELS,
    is_automation_issue,
    is_protected_file,
    issue_body_from_text,
    markdown_sections,
    parse_listish,
    parse_owned_files,
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
    scope_items = parse_listish(scope)
    if len(goal_items) > 1:
        return False
    combined = f"{goal}\n{scope}".lower()
    broad_markers = ["multiple outcomes", "several outcomes", "everything", "entire module", "broad refactor"]
    return not any(marker in combined for marker in broad_markers)


def validate_issue(text: str) -> dict[str, Any]:
    errors: list[str] = []
    warnings: list[str] = []
    body, metadata = issue_body_from_text(text)
    sections = markdown_sections(body)
    parsed: dict[str, Any] = {"sections": sorted(sections)}

    missing = [name for name in REQUIRED_SECTIONS if not section_lookup(sections, name)]
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

    owned_files = parse_owned_files(owned_text)
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
    production_files = [path for path in owned_files if not path.startswith(("tests/", "test/", "oracle/", "reports/")) and path not in {"CMakeLists.txt", "port_manifest.yaml", "ownership.yaml"}]
    test_oracle_files = [path for path in owned_files if path.startswith(("tests/", "test/", "oracle/", "reports/"))]
    example_files = [path for path in owned_files if path.startswith("examples/")]
    if len(production_files) > PRODUCTION_FILE_CAP:
        errors.append(f"production files exceed cap of {PRODUCTION_FILE_CAP}")
    if len(test_oracle_files) > TEST_ORACLE_FILE_CAP:
        errors.append(f"test/oracle files exceed cap of {TEST_ORACLE_FILE_CAP}")
    if len(example_files) > EXAMPLE_FILE_CAP:
        errors.append(f"example files exceed cap of {EXAMPLE_FILE_CAP}")

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
