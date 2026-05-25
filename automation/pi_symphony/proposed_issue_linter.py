from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence


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

REQUIRED_SECTIONS = (
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
)

SELECTOR_PREFIXES = (
    "- Manifest source selectors:",
    "- Manifest example selectors:",
    "- Repository path globs:",
    "- Common adjuncts:",
    "- Changed-file rule:",
)

ID_RE = re.compile(r"^P\d+\.\d+$")
HEADING_RE = re.compile(r"^##\s+(.+?)\s*$", re.MULTILINE)
META_RE = re.compile(r"^\*\*(?P<key>[^*]+):\*\*\s*(?P<value>.+?)\s*$", re.MULTILINE)
ISSUE_ID_RE = re.compile(r"^#\s+(P\d+\.\d+):", re.MULTILINE)


@dataclass(frozen=True)
class ProposedIssue:
    path: Path
    issue_id: str
    status: str
    validation_class: str
    blocked_by: tuple[str, ...]
    sections: tuple[str, ...]
    selector_lines: tuple[str, ...]
    number: int | None = None


@dataclass(frozen=True)
class LintMessage:
    path: Path
    message: str

    def format(self, root: Path) -> str:
        try:
            display = self.path.relative_to(root)
        except ValueError:
            display = self.path
        return f"{display}: {self.message}"


def issue_files(root: Path) -> list[Path]:
    if not root.exists():
        return []
    return sorted(
        path
        for path in root.rglob("*.md")
        if path.name not in {"README.md", "VALIDATION-GUIDE.md"}
    )


def load_issues(root: Path) -> list[ProposedIssue]:
    return [parse_issue(path) for path in issue_files(root)]


def load_github_issues(repo: str) -> list[ProposedIssue]:
    issues: list[ProposedIssue] = []
    for page in range(1, 11):
        result = subprocess.run(
            [
                "gh",
                "api",
                "-X",
                "GET",
                f"repos/{repo}/issues",
                "-f",
                "state=open",
                "-f",
                "per_page=100",
                "-f",
                f"page={page}",
            ],
            check=True,
            text=True,
            capture_output=True,
        )
        page_items = json.loads(result.stdout)
        for item in page_items:
            if "pull_request" in item:
                continue
            body = item.get("body") or ""
            title = item.get("title") or ""
            if "<!-- generated-local-issue -->" not in body and not re.search(r"\[P\d+\.\d+\]", title):
                continue
            issues.append(parse_issue_text(Path(f"github-issue-{item['number']}.md"), body, number=int(item["number"])))
        if len(page_items) < 100:
            break
    return sorted(issues, key=lambda issue: issue.issue_id)


def parse_issue(path: Path) -> ProposedIssue:
    return parse_issue_text(path, path.read_text())


def parse_issue_text(path: Path, text: str, *, number: int | None = None) -> ProposedIssue:
    title_match = ISSUE_ID_RE.search(text)
    metadata = {match.group("key").strip(): match.group("value").strip() for match in META_RE.finditer(text)}
    return ProposedIssue(
        path=path,
        issue_id=title_match.group(1) if title_match else "",
        status=metadata.get("Status", ""),
        validation_class=metadata.get("Validation class", ""),
        blocked_by=parse_blockers(metadata.get("Blocked by", "")),
        sections=tuple(match.group(1).strip() for match in HEADING_RE.finditer(text)),
        selector_lines=tuple(
            line.strip()
            for line in text.splitlines()
            if line.strip().startswith(SELECTOR_PREFIXES)
        ),
        number=number,
    )


def parse_blockers(value: str) -> tuple[str, ...]:
    value = value.strip()
    if not value or value.lower() == "none":
        return ()
    return tuple(part.strip() for part in value.split(",") if part.strip())


def lint_issues(issues: Sequence[ProposedIssue]) -> list[LintMessage]:
    messages: list[LintMessage] = []
    known_ids = {issue.issue_id for issue in issues if issue.issue_id}

    for issue in issues:
        if not issue.issue_id:
            messages.append(LintMessage(issue.path, "missing issue id in title"))
        elif issue.number is None and issue.issue_id not in issue.path.name:
            messages.append(LintMessage(issue.path, f"file name does not include issue id {issue.issue_id}"))

        if issue.validation_class not in VALIDATION_CLASSES:
            messages.append(LintMessage(issue.path, f"unknown validation class {issue.validation_class!r}"))

        missing_sections = [section for section in REQUIRED_SECTIONS if section not in issue.sections]
        if missing_sections:
            messages.append(LintMessage(issue.path, "missing required sections: " + ", ".join(missing_sections)))

        for blocker in issue.blocked_by:
            if not ID_RE.fullmatch(blocker):
                messages.append(LintMessage(issue.path, f"blocked-by entry is not an explicit issue id: {blocker!r}"))
            elif blocker not in known_ids:
                messages.append(LintMessage(issue.path, f"blocked-by entry does not match a local issue: {blocker}"))

        selector_names = {line.split(":", 1)[0].removeprefix("- ") + ":" for line in issue.selector_lines}
        missing_selectors = [prefix for prefix in SELECTOR_PREFIXES if prefix.removeprefix("- ") not in selector_names]
        if missing_selectors:
            messages.append(LintMessage(issue.path, "missing owned-file selector lines"))

        for line in issue.selector_lines:
            if _selector_contains_unparseable_prose(line):
                messages.append(LintMessage(issue.path, f"owned-file selector contains unparseable prose: {line}"))

    return messages


def lint_issue_map(issue_map_path: Path, issues: Sequence[ProposedIssue]) -> list[LintMessage]:
    """Validate that the GitHub issue map does not preserve stale blocker metadata."""
    if not issue_map_path.exists():
        return []

    messages: list[LintMessage] = []
    issue_map = json.loads(issue_map_path.read_text())
    by_id = {issue.issue_id: issue for issue in issues if issue.issue_id}
    known_ids = set(by_id)

    for entry in issue_map.get("created", []):
        issue_id = str(entry.get("id", ""))
        issue = by_id.get(issue_id)
        if not issue:
            messages.append(LintMessage(issue_map_path, f"issue-map entry does not match a local issue: {issue_id!r}"))
            continue

        mapped_blockers = parse_blockers(str(entry.get("blocked", "")))
        for blocker in mapped_blockers:
            if not ID_RE.fullmatch(blocker):
                messages.append(LintMessage(issue_map_path, f"issue-map blocked entry for {issue_id} is not an explicit issue id: {blocker!r}"))
            elif blocker not in known_ids:
                messages.append(LintMessage(issue_map_path, f"issue-map blocked entry for {issue_id} does not match a local issue: {blocker}"))

        if mapped_blockers != issue.blocked_by:
            expected = ", ".join(issue.blocked_by) if issue.blocked_by else "None"
            actual = ", ".join(mapped_blockers) if mapped_blockers else "None"
            messages.append(LintMessage(issue_map_path, f"issue-map blockers for {issue_id} do not match local issue: {actual} != {expected}"))

    return messages


def github_label_updates(issue_map_path: Path, issues: Sequence[ProposedIssue]) -> list[dict[str, object]]:
    """Return minimal label updates that keep only unblocked issues ready."""
    if all(issue.number is not None for issue in issues):
        issue_entries = [
            {"id": issue.issue_id, "number": issue.number}
            for issue in issues
        ]
    else:
        issue_map = json.loads(issue_map_path.read_text())
        issue_entries = issue_map.get("created", [])

    by_id = {issue.issue_id: issue for issue in issues}
    updates: list[dict[str, object]] = []
    for entry in issue_entries:
        issue = by_id.get(str(entry.get("id", "")))
        if not issue:
            continue
        blocked = bool(issue.blocked_by)
        updates.append(
            {
                "id": issue.issue_id,
                "number": int(entry["number"]),
                "blocked": blocked,
                "remove": ["ai:ready"] if blocked else ["ai:blocked"],
                "add": ["ai:blocked"] if blocked else ["ai:ready"],
            }
        )
    return updates


def apply_github_label_updates(updates: Iterable[dict[str, object]], *, repo: str) -> None:
    import subprocess

    for update in updates:
        number = str(update["number"])
        for label in update["remove"]:
            subprocess.run(
                ["gh", "issue", "edit", number, "--repo", repo, "--remove-label", str(label)],
                check=False,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
        add_labels = [str(label) for label in update["add"]]
        if add_labels:
            subprocess.run(
                ["gh", "issue", "edit", number, "--repo", repo, "--add-label", ",".join(add_labels)],
                check=True,
            )


def _selector_contains_unparseable_prose(line: str) -> bool:
    if line.endswith("Changed-file rule: every modified path must match these selectors or the named adjunct set; otherwise update this issue before implementation."):
        return False
    _, _, value = line.partition(":")
    value = value.strip()
    if not value or value == "none":
        return False
    forbidden = (" plus ", " unless ", " except ", " read-only ", " named by ")
    return any(token in f" {value} " for token in forbidden)


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Lint and gate proposed GitHub issues.")
    parser.add_argument("--root", type=Path, default=Path("docs/proposed-issues"))
    parser.add_argument("--source", choices=("auto", "local", "github"), default="auto")
    parser.add_argument("--issue-map", type=Path, default=Path("docs/proposed-issues/github-issue-map.json"))
    parser.add_argument("--github-label-plan", action="store_true", help="Print JSON label changes for GitHub readiness.")
    parser.add_argument("--apply-github-labels", action="store_true", help="Apply GitHub readiness labels with gh.")
    parser.add_argument("--repo", default="michelreifenrath/pyqtgraph_to_cpp")
    args = parser.parse_args(argv)

    if args.source == "github" or (args.source == "auto" and not issue_files(args.root)):
        issues = load_github_issues(args.repo)
        issue_map_messages: list[LintMessage] = []
    else:
        issues = load_issues(args.root)
        issue_map_messages = lint_issue_map(args.issue_map, issues)

    messages = lint_issues(issues)
    messages.extend(issue_map_messages)
    if messages:
        for message in messages:
            print(message.format(Path.cwd()), file=sys.stderr)
        return 1

    if args.github_label_plan or args.apply_github_labels:
        updates = github_label_updates(args.issue_map, issues)
        if args.github_label_plan:
            print(json.dumps(updates, indent=2))
        if args.apply_github_labels:
            apply_github_label_updates(updates, repo=args.repo)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
