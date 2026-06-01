from __future__ import annotations

import fnmatch
import json
import re
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]

LEGACY_REQUIRED_SECTIONS = [
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
REQUIRED_SECTIONS = LEGACY_REQUIRED_SECTIONS
GENERATED_REQUIRED_SECTIONS = [
    "Goal",
    "Current evidence",
    "Scope",
    "Owned files",
    "Required local proof",
    "TDD plan",
    "Validation commands",
    "Acceptance criteria",
    "Done definition",
    "Scope boundaries",
]
SELECTOR_PREFIXES = (
    "Manifest source selectors",
    "Manifest example selectors",
    "Repository path globs",
    "Common adjuncts",
    "Changed-file rule",
)
VALIDATION_CLASSES = {
    "api-runtime",
    "core-oracle",
    "decision-doc",
    "example-port",
    "exporter-io",
    "interaction-ui",
    "manifest-infra",
    "opengl-render",
    "oracle-infra",
    "package-consumer",
    "performance",
    "pixel-image",
    "resource-assets",
    "review-approval",
    "rollup-final",
    "script-infra",
    "visual-render",
}
VALIDATION_LEVELS = {"required", "not_applicable"}
PROTECTED_PATTERNS = [
    "MISSION.md",
    "FACTORY_RULES.md",
    "AGENTS.md",
    "WORKFLOW.md",
    ".archon/**",
    "scripts/factory/**",
    "archive/**",
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


def metadata_fields(body: str) -> dict[str, str]:
    return {
        match.group("key").strip(): match.group("value").strip()
        for match in re.finditer(r"^\*\*(?P<key>[^*]+):\*\*\s*(?P<value>.+?)\s*$", body, re.MULTILINE)
    }


def parse_blockers(value: str) -> list[str]:
    value = value.strip()
    if not value or value.lower() == "none":
        return []
    return [part.strip() for part in value.split(",") if part.strip()]


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


def split_selector_values(value: str) -> list[str]:
    value = value.strip()
    if not value or value.lower() in {"none", "n/a", "not_applicable"}:
        return []
    parts = re.split(r";|,", value)
    return [part.strip().strip("` ") for part in parts if part.strip().strip("` ")]


def normalize_path(path: str) -> str:
    normalized = path.strip()
    while normalized.startswith("./"):
        normalized = normalized[2:]
    while normalized.startswith("/"):
        normalized = normalized[1:]
    return normalized


def parse_owned_files(text: str) -> list[str]:
    files: list[str] = []
    for item in parse_listish(text):
        if item.lower() in {"none", "n/a", "not_applicable"}:
            continue
        files.append(normalize_path(item))
    return files


def parse_owned_selectors(text: str) -> dict[str, list[str]]:
    selectors = {
        "manifest_source": [],
        "manifest_examples": [],
        "repository_globs": [],
        "common_adjuncts": [],
        "changed_file_rule": [],
    }
    mapping = {
        "manifest source selectors": "manifest_source",
        "manifest example selectors": "manifest_examples",
        "repository path globs": "repository_globs",
        "common adjuncts": "common_adjuncts",
        "changed-file rule": "changed_file_rule",
    }
    for raw in text.splitlines():
        line = raw.strip()
        line = re.sub(r"^[-*]\s+", "", line)
        if ":" not in line:
            continue
        prefix, value = line.split(":", 1)
        key = mapping.get(prefix.strip().lower())
        if key:
            selectors[key].extend(split_selector_values(value))
    return selectors


def has_generated_owned_selectors(text: str) -> bool:
    return any(line.strip().lstrip("-* ").startswith(prefix + ":") for line in text.splitlines() for prefix in SELECTOR_PREFIXES)


def is_generated_local_issue(body: str, metadata: dict[str, Any] | None = None) -> bool:
    title = str((metadata or {}).get("title", ""))
    haystack = f"{title}\n{body}"
    if "<!-- generated-local-issue -->" in haystack:
        return True
    fields = metadata_fields(body)
    sections = markdown_sections(body)
    return bool(fields.get("Validation class") and section_lookup(sections, "Required local proof"))


def issue_id_from_text(body: str, metadata: dict[str, Any] | None = None) -> str:
    title = str((metadata or {}).get("title", ""))
    for source in (body, title, str((metadata or {}).get("path", ""))):
        match = re.search(r"(?:^#\s+|\[)(P\d+\.\d+)(?::|\])", source, re.MULTILINE)
        if match:
            return match.group(1)
    return ""


def normalize_label_names(labels: Any) -> list[str]:
    names: list[str] = []
    if not isinstance(labels, list):
        return names
    for label in labels:
        if isinstance(label, dict):
            name = str(label.get("name", ""))
        else:
            name = str(label)
        if name:
            names.append(name)
    return names


def is_automation_issue(body: str, metadata: dict[str, Any] | None = None) -> bool:
    labels = " ".join(normalize_label_names((metadata or {}).get("labels", [])))
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


def _load_manifest() -> dict[str, Any]:
    manifest_path = ROOT / "port_manifest.yaml"
    if not manifest_path.exists():
        return {}
    try:
        import yaml

        loaded = yaml.safe_load(manifest_path.read_text(encoding="utf-8"))
        return loaded if isinstance(loaded, dict) else {}
    except Exception:
        return {}


def _selector_matches_upstream(selector: str, upstream_path: str) -> bool:
    selector = selector.strip().lstrip("/")
    upstream_path = upstream_path.strip().lstrip("/")
    variants = {selector}
    if not selector.startswith("pyqtgraph/"):
        variants.add("pyqtgraph/" + selector)
        variants.add("pyqtgraph/examples/" + selector)
    return any(
        upstream_path == variant
        or upstream_path.endswith("/" + selector)
        or fnmatch.fnmatch(upstream_path, variant)
        or fnmatch.fnmatch(upstream_path, selector)
        for variant in variants
    )


def manifest_paths_for_selectors(source_selectors: list[str], example_selectors: list[str]) -> tuple[set[str], set[str]]:
    manifest = _load_manifest()
    paths: set[str] = set()
    components: set[str] = set()
    for record in manifest.get("source_files", []) or []:
        upstream = str(record.get("upstream_path", ""))
        if not any(_selector_matches_upstream(selector, upstream) for selector in source_selectors):
            continue
        for key in ("target_header_path", "target_source_path"):
            value = str(record.get(key, "")).strip()
            if value:
                paths.add(normalize_path(value))
        if upstream:
            components.add(Path(upstream).stem.lower())
    for record in manifest.get("examples", []) or []:
        upstream = str(record.get("upstream_path", ""))
        name = str(record.get("name", ""))
        if not any(_selector_matches_upstream(selector, upstream) or selector == name for selector in example_selectors):
            continue
        value = str(record.get("target_source_path", "")).strip()
        if value:
            paths.add(normalize_path(value))
        if upstream:
            components.add(Path(upstream).stem.lower())
        if name:
            components.add(name.lower())
    return paths, components


def issue_tokens(issue_id: str) -> set[str]:
    if not issue_id:
        return set()
    return {issue_id, issue_id.replace(".", "_"), issue_id.lower(), issue_id.replace(".", "_").lower()}


def parse_issue_ownership(body: str, metadata: dict[str, Any] | None = None) -> dict[str, Any]:
    sections = markdown_sections(body)
    owned_text = section_lookup(sections, "Owned files")
    issue_id = issue_id_from_text(body, metadata)
    selectors = parse_owned_selectors(owned_text)
    if any(selectors[key] for key in ("manifest_source", "manifest_examples", "repository_globs", "common_adjuncts")):
        manifest_paths, components = manifest_paths_for_selectors(selectors["manifest_source"], selectors["manifest_examples"])
        repo_globs = [normalize_path(value) for value in selectors["repository_globs"]]
        adjuncts = [value.strip().lower() for value in selectors["common_adjuncts"]]
        direct_repo_paths = {
            value
            for value in repo_globs
            if (not any(ch in value for ch in "*?[") and "/" in value)
            or value in {"CMakeLists.txt", "CMakePresets.json", "port_manifest.yaml", "ownership.yaml"}
        }
        components.update(Path(path).stem.lower() for path in manifest_paths)
        components.update(Path(path).stem.lower() for path in direct_repo_paths)
        return {
            "mode": "selectors",
            "owned_files": sorted(manifest_paths | direct_repo_paths),
            "repository_globs": repo_globs,
            "common_adjuncts": adjuncts,
            "issue_id": issue_id,
            "owned_components": sorted(component for component in components if component),
            "selectors": selectors,
        }
    owned_files = parse_owned_files(owned_text)
    return {
        "mode": "literal",
        "owned_files": owned_files,
        "repository_globs": [],
        "common_adjuncts": [],
        "issue_id": issue_id,
        "owned_components": sorted(Path(path).stem.lower() for path in owned_files),
        "selectors": selectors,
    }


def adjunct_matches(path: str, adjuncts: list[str], issue_id: str, owned_components: list[str]) -> bool:
    normalized = normalize_path(path)
    lower = normalized.lower()
    tokens = {token.lower() for token in issue_tokens(issue_id)}
    components = {component.lower() for component in owned_components if component}

    def contains_issue_or_component() -> bool:
        return any(token in lower for token in tokens) or any(component and component in lower for component in components)

    if "focused-tests" in adjuncts:
        if normalized.startswith("tests/") and contains_issue_or_component():
            return True
        if normalized.startswith("oracle/probes/") and any(token in lower for token in tokens):
            return True
        if normalized.startswith("oracle/") and any(token in lower for token in tokens):
            return True
        if issue_id and normalized.startswith(f"reports/issues/{issue_id}/"):
            return True
    if "focused-visual" in adjuncts:
        if adjunct_matches(path, ["focused-tests"], issue_id, owned_components):
            return True
        if normalized.startswith("tests/visual/"):
            return True
        if issue_id and normalized.startswith(f"reports/visual/{issue_id}/"):
            return True
        if normalized.startswith("scripts/") and "visual" in lower:
            return True
    if "focused-examples" in adjuncts:
        if normalized.startswith("tests/examples/"):
            return True
        if fnmatch.fnmatch(normalized, "scripts/run_*examples*"):
            return True
        if issue_id and normalized.startswith(f"reports/examples/{issue_id}/"):
            return True
    if "focused-doc-report" in adjuncts:
        if issue_id and normalized.startswith(f"reports/issues/{issue_id}/"):
            return True
    if "build-plumbing" in adjuncts:
        if normalized in {"CMakeLists.txt", "tests/CMakeLists.txt", "CMakePresets.json"}:
            return True
        if normalized.startswith("cmake/"):
            return True
        if issue_id and normalized.startswith(f"reports/issues/{issue_id}/"):
            return True
    return False


def classify_changed_file(
    path: str,
    owned: set[str],
    repository_globs: list[str] | None = None,
    common_adjuncts: list[str] | None = None,
    issue_id: str = "",
    owned_components: list[str] | None = None,
) -> str:
    normalized = normalize_path(path)
    if normalized in owned:
        return "owned"
    if any(fnmatch.fnmatch(normalized, pattern) for pattern in (repository_globs or [])):
        return "owned"
    if adjunct_matches(normalized, common_adjuncts or [], issue_id, owned_components or []):
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
