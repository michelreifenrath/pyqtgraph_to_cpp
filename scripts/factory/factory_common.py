from __future__ import annotations

import fnmatch
import json
import re
import sys
from pathlib import Path
from typing import Any

REQUIRED_SECTIONS = [
    "Goal",
    "PyQtGraph reference",
    "Dependencies",
    "Owned files",
    "Scope",
    "TDD plan",
    "Validation level",
    "Validation commands",
    "Done definition",
]
VALIDATION_LEVELS = {"required", "not_applicable"}
PROTECTED_PATTERNS = [
    "MISSION.md",
    "FACTORY_RULES.md",
    "AGENTS.md",
    "WORKFLOW.md",
    "docs/pyqtgraph-cpp-port-workflow.md",
    ".archon/**",
    "scripts/factory/**",
    ".env*",
    "**/.env*",
]
SHARED_INTEGRATION_ALLOWLIST = {
    "CMakeLists.txt",
    "tests/CMakeLists.txt",
    "port_manifest.yaml",
    "ownership.yaml",
}
TOTAL_FILE_CAP = 10
PRODUCTION_FILE_CAP = 4
TEST_ORACLE_FILE_CAP = 4
SHARED_INTEGRATION_CAP = 3
EXAMPLE_FILE_CAP = 1


def read_text_arg(path: str | None) -> str:
    if path:
        return Path(path).read_text(encoding="utf-8")
    return sys.stdin.read()


def issue_body_from_text(text: str) -> tuple[str, dict[str, Any]]:
    raw = text.strip()
    metadata: dict[str, Any] = {}
    if raw.startswith("{"):
        data = json.loads(raw)
        if not isinstance(data, dict):
            raise ValueError("issue JSON must be an object")
        metadata = data
        return str(data.get("body", "")), metadata
    return text, metadata


def markdown_sections(body: str) -> dict[str, str]:
    sections: dict[str, list[str]] = {}
    current: str | None = None
    for line in body.splitlines():
        match = re.match(r"^#{1,6}\s+(.+?)\s*$", line)
        if match:
            current = match.group(1).strip().rstrip(":")
            sections.setdefault(current, [])
            continue
        if current is not None:
            sections[current].append(line)
    return {key: "\n".join(value).strip() for key, value in sections.items()}


def section_lookup(sections: dict[str, str], name: str) -> str:
    wanted = normalize_heading(name)
    for key, value in sections.items():
        if normalize_heading(key) == wanted:
            return value.strip()
    return ""


def normalize_heading(value: str) -> str:
    return re.sub(r"[^a-z0-9]+", " ", value.lower()).strip()


def parse_listish(text: str) -> list[str]:
    values: list[str] = []
    for raw in text.splitlines():
        line = raw.strip()
        if not line:
            continue
        line = re.sub(r"^[-*]\s+", "", line)
        line = re.sub(r"^\d+[.)]\s+", "", line)
        line = line.strip("` ")
        if line:
            values.append(line)
    if not values and text.strip():
        values = [part.strip("` ") for part in re.split(r"[,\n]", text) if part.strip()]
    return values


def normalize_path(path: str) -> str:
    return path.strip().lstrip("./")


def parse_owned_files(text: str) -> list[str]:
    files: list[str] = []
    for item in parse_listish(text):
        if item.lower() in {"none", "n/a", "not_applicable"}:
            continue
        files.append(normalize_path(item))
    return files


def is_automation_issue(body: str, metadata: dict[str, Any] | None = None) -> bool:
    labels = " ".join(str(label) for label in (metadata or {}).get("labels", []))
    title = str((metadata or {}).get("title", ""))
    haystack = f"{title}\n{labels}\n{body}".lower()
    markers = [
        "automation issue",
        "governance issue",
        "factory issue",
        "automation/governance",
        "type: automation",
        "type: governance",
        "factory:automation",
        "factory:governance",
    ]
    return any(marker in haystack for marker in markers)


def is_protected_file(path: str) -> bool:
    normalized = normalize_path(path)
    return any(fnmatch.fnmatch(normalized, pattern) for pattern in PROTECTED_PATTERNS)


def classify_changed_file(path: str, owned: set[str]) -> str:
    normalized = normalize_path(path)
    if normalized in owned:
        return "owned"
    if normalized in SHARED_INTEGRATION_ALLOWLIST:
        return "shared_integration"
    return "outside_scope"


def parse_validation_levels(text: str) -> dict[str, str]:
    levels: dict[str, str] = {}
    for raw in text.splitlines():
        line = raw.strip().strip("-* ")
        if not line:
            continue
        match = re.match(r"(?i)^(numeric|visual|interaction)\s*[:=-]\s*([a-z_ -]+)$", line)
        if match:
            levels[match.group(1).lower()] = match.group(2).strip().lower().replace("-", "_").replace(" ", "_")
    if not levels and text.strip().lower() in VALIDATION_LEVELS:
        value = text.strip().lower()
        levels = {"numeric": value, "visual": value, "interaction": value}
    return levels


def print_json(payload: dict[str, Any]) -> None:
    print(json.dumps(payload, indent=2, sort_keys=True))
