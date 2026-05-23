from __future__ import annotations

import argparse
import json
import re
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from automation.pi_symphony.config import load_workflow

STATE_LABEL_PREFIX = "ai:"
HUMAN_LABEL = "human-review"
TENANT_LABEL_PREFIX = "tenant:"
TAG_LABEL_PREFIX = "tag:"

DOMAIN_TAGS: dict[str, str] = {
    "PGBOOT": "tag:bootstrap",
    "PGINV": "tag:inventory",
    "PGORACLE": "tag:oracle",
    "PGCORE": "tag:core",
    "PGGI": "tag:graphics",
    "PGSCENE": "tag:graphics",
    "PGVIEW": "tag:graphics",
    "PGPLOT": "tag:plot",
    "PGEXAMPLE": "tag:examples",
}

LABEL_DEFINITIONS: dict[str, tuple[str, str]] = {
    "tag:bootstrap": ("bfdadc", "Bootstrap and automation setup."),
    "tag:inventory": ("bfdadc", "Inventory and manifest work."),
    "tag:oracle": ("bfdadc", "Validation oracle work."),
    "tag:core": ("bfdadc", "Core data/model helpers."),
    "tag:graphics": ("bfdadc", "Graphics scene/item/view work."),
    "tag:plot": ("bfdadc", "Plotting widget/item work."),
    "tag:examples": ("bfdadc", "Examples and smoke validation work."),
}


@dataclass(frozen=True)
class SimplifiedIssue:
    number: int
    title: str
    old_body_chars: int
    new_body_chars: int
    old_labels: list[str]
    new_labels: list[str]

    @property
    def removed_labels(self) -> list[str]:
        return [label for label in self.old_labels if label not in self.new_labels]

    @property
    def added_labels(self) -> list[str]:
        return [label for label in self.new_labels if label not in self.old_labels]


SECTION_RE = re.compile(r"^##\s+(.+?)\s*$", re.MULTILINE)


def compact_issue_body(title: str, body: str, *, max_chars: int = 1200) -> str:
    goal = _first_paragraph(_section(body, "Goal")) or _title_without_ai_prefix(title)
    dependencies = _bullet_lines(_section(body, "Dependencies")) or ["- none"]
    owned_files = _bullet_lines(_section(body, "Owned files")) or ["- see implementation scope"]
    validation = _validation_lines(body) or ["- `scripts/gate commit`", "- `python3 -m pytest -q`"]
    issue_code = _issue_code(title)
    report_line = f"- Implementation report: `reports/agents/{issue_code}.md`" if issue_code else "- Implementation report written where applicable."

    compact = "\n".join(
        [
            "## Goal",
            goal,
            "",
            "## Dependencies",
            *dependencies,
            "",
            "## Owned files",
            *owned_files,
            "",
            "## Validation",
            *validation,
            "",
            "## Done",
            "- Focused checks pass before handoff.",
            "- Scope stays within owned files.",
            report_line,
            "- PR opened by automation, or handoff explains why no PR was opened.",
            "",
        ]
    )
    if len(compact) <= max_chars:
        return compact
    return _shorten_owned_files(compact, max_chars)


def simplified_labels(title: str, labels: list[str]) -> list[str]:
    result: list[str] = []
    for label in labels:
        if label.startswith(STATE_LABEL_PREFIX) or label == HUMAN_LABEL:
            if label not in result:
                result.append(label)
    domain = _domain_tag(title)
    if domain and domain not in result:
        result.append(domain)
    return result


def simplify_issue_payload(issue: dict[str, Any], *, max_body_chars: int) -> SimplifiedIssue:
    labels = [label["name"] if isinstance(label, dict) else str(label) for label in issue.get("labels", [])]
    body = str(issue.get("body") or "")
    new_body = compact_issue_body(str(issue.get("title") or ""), body, max_chars=max_body_chars)
    return SimplifiedIssue(
        number=int(issue["number"]),
        title=str(issue.get("title") or ""),
        old_body_chars=len(body),
        new_body_chars=len(new_body),
        old_labels=labels,
        new_labels=simplified_labels(str(issue.get("title") or ""), labels),
    )


def _section(body: str, name: str) -> str:
    matches = list(SECTION_RE.finditer(body))
    for index, match in enumerate(matches):
        if match.group(1).strip().lower() != name.lower():
            continue
        start = match.end()
        end = matches[index + 1].start() if index + 1 < len(matches) else len(body)
        return body[start:end].strip()
    return ""


def _first_paragraph(text: str) -> str:
    lines: list[str] = []
    for raw in text.splitlines():
        line = raw.strip()
        if not line:
            if lines:
                break
            continue
        if line.startswith("-") or line.startswith("```"):
            break
        lines.append(line)
    return " ".join(lines).strip()


def _bullet_lines(text: str) -> list[str]:
    lines: list[str] = []
    for raw in text.splitlines():
        line = raw.strip()
        if line.startswith("- "):
            lines.append(line)
    return lines


def _validation_lines(body: str) -> list[str]:
    text = _section(body, "Validation commands") or _section(body, "Validation")
    commands: list[str] = []
    in_fence = False
    for raw in text.splitlines():
        line = raw.strip()
        if line.startswith("```"):
            in_fence = not in_fence
            continue
        if in_fence and line:
            commands.append(line)
            continue
        if line.startswith("- `") and line.endswith("`"):
            commands.append(line[3:-1])
    return [f"- `{command}`" for command in dict.fromkeys(commands)]


def _shorten_owned_files(text: str, max_chars: int) -> str:
    if len(text) <= max_chars:
        return text
    lines = text.splitlines()
    output: list[str] = []
    owned_count = 0
    in_owned = False
    for line in lines:
        if line == "## Owned files":
            in_owned = True
            output.append(line)
            continue
        if in_owned and line.startswith("## "):
            in_owned = False
        if in_owned and line.startswith("- "):
            owned_count += 1
            if owned_count > 8:
                if owned_count == 9:
                    output.append("- additional owned files omitted for brevity")
                continue
        output.append(line)
    shortened = "\n".join(output) + "\n"
    if len(shortened) <= max_chars:
        return shortened
    return shortened[: max_chars - 2].rstrip() + "…\n"


def _title_without_ai_prefix(title: str) -> str:
    title = title.strip()
    if title.startswith("[AI] "):
        title = title[5:]
    if ": " in title:
        return title.split(": ", 1)[1].strip()
    return title


def _issue_code(title: str) -> str:
    match = re.search(r"\b(PG[A-Z]+-\d+)\b", title)
    return match.group(1) if match else ""


def _domain_tag(title: str) -> str | None:
    code = _issue_code(title)
    if not code:
        return None
    prefix = code.split("-", 1)[0]
    return DOMAIN_TAGS.get(prefix)


def _run_json(args: list[str]) -> Any:
    return json.loads(subprocess.check_output(args, text=True))


def _run(args: list[str]) -> None:
    subprocess.run(args, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)


def _open_issues(repo: str) -> list[dict[str, Any]]:
    data = _run_json([
        "gh",
        "issue",
        "list",
        "--repo",
        repo,
        "--state",
        "open",
        "--limit",
        "200",
        "--json",
        "number,title,body,labels,url",
    ])
    return [item for item in data if "/pull/" not in str(item.get("url") or "")]


def ensure_domain_labels(repo: str) -> None:
    existing = {item["name"] for item in _run_json(["gh", "label", "list", "--repo", repo, "--limit", "500", "--json", "name"])}
    for label, (color, description) in LABEL_DEFINITIONS.items():
        if label in existing:
            continue
        _run(["gh", "label", "create", label, "--repo", repo, "--color", color, "--description", description])


def apply_simplification(repo: str, issue: dict[str, Any], *, max_body_chars: int) -> SimplifiedIssue:
    simplified = simplify_issue_payload(issue, max_body_chars=max_body_chars)
    new_body = compact_issue_body(str(issue.get("title") or ""), str(issue.get("body") or ""), max_chars=max_body_chars)
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", delete=False) as handle:
        handle.write(new_body)
        path = handle.name
    try:
        _run(["gh", "issue", "edit", str(simplified.number), "--repo", repo, "--body-file", path])
    finally:
        Path(path).unlink(missing_ok=True)
    for label in simplified.removed_labels:
        _run(["gh", "issue", "edit", str(simplified.number), "--repo", repo, "--remove-label", label])
    if simplified.added_labels:
        _run(["gh", "issue", "edit", str(simplified.number), "--repo", repo, "--add-label", ",".join(simplified.added_labels)])
    return simplified


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Compact Pi Symphony GitHub issue bodies and labels.")
    parser.add_argument("--workflow", default="WORKFLOW.md")
    parser.add_argument("--apply", action="store_true", help="Apply changes to GitHub. Omit for dry-run.")
    parser.add_argument("--json", action="store_true", help="Print JSON summary.")
    args = parser.parse_args(argv)

    config = load_workflow(args.workflow)
    repo = config.tracker.repo
    issues = _open_issues(repo)
    max_chars = config.github_output.issue_body_max_chars
    summaries = [simplify_issue_payload(issue, max_body_chars=max_chars) for issue in issues]
    if args.apply:
        ensure_domain_labels(repo)
        summaries = [apply_simplification(repo, issue, max_body_chars=max_chars) for issue in issues]

    data = [
        {
            "number": item.number,
            "title": item.title,
            "old_body_chars": item.old_body_chars,
            "new_body_chars": item.new_body_chars,
            "old_labels": item.old_labels,
            "new_labels": item.new_labels,
            "removed_labels": item.removed_labels,
            "added_labels": item.added_labels,
        }
        for item in summaries
    ]
    if args.json:
        print(json.dumps(data, indent=2))
    else:
        action = "applied" if args.apply else "dry-run"
        print(f"{action}: {len(data)} issues")
        for item in data:
            print(
                f"#{item['number']} body {item['old_body_chars']}->{item['new_body_chars']} "
                f"labels {item['old_labels']} -> {item['new_labels']}"
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
