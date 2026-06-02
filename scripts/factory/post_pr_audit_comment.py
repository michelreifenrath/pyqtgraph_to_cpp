#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any

MARKER_PREFIX = "<!-- dark-factory-validation:"


def _load_json(path: Path, default: Any = None) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return default


def _bool_icon(value: Any) -> str:
    return "pass" if value is True else "fail"


def _short(value: Any, length: int = 8) -> str:
    text = str(value or "")
    return text[:length] if text else "unknown"


def _as_list(value: Any) -> list[str]:
    if value is None or value is False or value == "":
        return []
    if isinstance(value, list):
        return [str(item).strip() for item in value if str(item).strip()]
    if isinstance(value, dict):
        for key in ("findings", "blocking_findings", "errors"):
            if isinstance(value.get(key), list):
                return _as_list(value[key])
        summary = value.get("summary") or value.get("reasoning") or value.get("reason")
        return [str(summary).strip()] if summary else []
    return [str(value).strip()]


def collect_findings(verdict: dict[str, Any], merge_result: dict[str, Any]) -> list[str]:
    findings: list[str] = []
    for key in ("findings", "blocking_findings", "review_findings", "holdout_findings", "deterministic_guard_errors"):
        findings.extend(_as_list(verdict.get(key)))
    findings.extend(_as_list(merge_result.get("errors")))
    reason = merge_result.get("reason") or verdict.get("reasoning") or verdict.get("reason")
    if reason and not findings:
        findings.append(str(reason))

    compact: list[str] = []
    seen: set[str] = set()
    for item in findings:
        line = " ".join(str(item).split())
        if not line or line in seen:
            continue
        seen.add(line)
        compact.append(line[:220])
    return compact[:3]


def build_comment(artifacts_dir: Path, workflow_id: str | None = None) -> str:
    verdict = _load_json(artifacts_dir / "verdict.json", {}) or {}
    merge_result = _load_json(artifacts_dir / "merge-result.json", {}) or {}
    pr = _load_json(artifacts_dir / "pr.json", {}) or {}
    if not isinstance(verdict, dict):
        verdict = {}
    if not isinstance(merge_result, dict):
        merge_result = {}
    if not isinstance(pr, dict):
        pr = {}

    run_id = workflow_id or os.environ.get("WORKFLOW_ID") or artifacts_dir.name
    decision = str(merge_result.get("decision") or verdict.get("decision") or "unknown")
    label = {
        "merge": "auto-merge approved",
        "fix": "rework required",
        "human-review": "human-review required",
    }.get(decision, decision)

    gate_pairs = [
        ("readiness", verdict.get("readiness")),
        ("scope", verdict.get("scope")),
        ("tests", verdict.get("tests")),
        ("autoreview", verdict.get("autoreview")),
        ("holdout", verdict.get("holdout")),
        ("diff", verdict.get("diff_check")),
    ]
    gates = ", ".join(f"{name} { _bool_icon(value) }" for name, value in gate_pairs)

    findings = collect_findings(verdict, merge_result)
    if findings:
        finding_lines = "\n".join(f"- {item}" for item in findings)
    else:
        finding_lines = "- none"

    if decision == "merge":
        next_step = "merged automatically or merge command approved."
    elif decision == "fix":
        next_step = "factory should apply a scoped fix and re-run validation."
    elif decision == "human-review":
        next_step = "factory stopped fail-closed for human review."
    else:
        next_step = "inspect workflow artifacts."

    marker = f"{MARKER_PREFIX}{run_id} -->"
    return "\n".join(
        [
            marker,
            f"🤖 Factory validation: {label}",
            "",
            f"Run: `{_short(run_id)}`  Head: `{_short(verdict.get('head_sha') or pr.get('headRefOid'))}`",
            f"Decision: `{decision}`",
            f"Gates: {gates}",
            "",
            "Findings:",
            finding_lines,
            "",
            f"Next: {next_step}",
        ]
    ) + "\n"


def post_or_update_comment(pr_number: int, body: str, repo: str | None = None) -> dict[str, Any]:
    marker = body.splitlines()[0]
    list_args = ["gh", "api", "--paginate", f"repos/{repo}/issues/{pr_number}/comments"] if repo else ["gh", "api", "--paginate", f"repos/{{owner}}/{{repo}}/issues/{pr_number}/comments"]
    existing = subprocess.run(list_args, text=True, capture_output=True, check=False)
    if existing.returncode != 0:
        return {"ok": False, "action": "list-failed", "stderr": existing.stderr}
    comments = json.loads(existing.stdout or "[]")
    for comment in comments:
        if marker in str(comment.get("body", "")):
            comment_id = comment.get("id")
            endpoint = f"repos/{repo}/issues/comments/{comment_id}" if repo else f"repos/{{owner}}/{{repo}}/issues/comments/{comment_id}"
            update = subprocess.run(["gh", "api", "--method", "PATCH", endpoint, "-f", f"body={body}"], text=True, capture_output=True, check=False)
            return {"ok": update.returncode == 0, "action": "updated", "comment_id": comment_id, "stderr": update.stderr}
    create_args = ["gh", "api", "--method", "POST", f"repos/{repo}/issues/{pr_number}/comments", "-f", f"body={body}"] if repo else ["gh", "api", "--method", "POST", f"repos/{{owner}}/{{repo}}/issues/{pr_number}/comments", "-f", f"body={body}"]
    created = subprocess.run(create_args, text=True, capture_output=True, check=False)
    payload = _load_json_from_text(created.stdout) if created.stdout else {}
    return {"ok": created.returncode == 0, "action": "created", "comment_id": payload.get("id"), "stderr": created.stderr}


def _load_json_from_text(text: str) -> dict[str, Any]:
    try:
        payload = json.loads(text)
        return payload if isinstance(payload, dict) else {}
    except Exception:
        return {}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Build and optionally post a concise Dark Factory PR audit comment.")
    parser.add_argument("--artifacts-dir", required=True)
    parser.add_argument("--workflow-id", default=os.environ.get("WORKFLOW_ID"))
    parser.add_argument("--repo", help="owner/repo for gh api; defaults to gh repo context")
    parser.add_argument("--post", action="store_true", help="Post/update the comment on GitHub")
    args = parser.parse_args(argv)

    artifacts_dir = Path(args.artifacts_dir)
    body = build_comment(artifacts_dir, workflow_id=args.workflow_id)
    (artifacts_dir / "factory-pr-audit-comment.md").write_text(body, encoding="utf-8")
    result: dict[str, Any] = {"ok": True, "posted": False, "comment_path": str(artifacts_dir / "factory-pr-audit-comment.md")}
    if args.post:
        pr = _load_json(artifacts_dir / "pr.json", {}) or {}
        verdict = _load_json(artifacts_dir / "verdict.json", {}) or {}
        pr_number = pr.get("number") or verdict.get("pr_number")
        if not pr_number:
            result = {"ok": False, "posted": False, "error": "missing PR number"}
        else:
            post_result = post_or_update_comment(int(pr_number), body, repo=args.repo)
            result = {**post_result, "posted": post_result.get("ok") is True, "comment_path": str(artifacts_dir / "factory-pr-audit-comment.md")}
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if result.get("ok") else 1


if __name__ == "__main__":
    sys.exit(main())
