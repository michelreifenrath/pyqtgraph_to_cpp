from __future__ import annotations

import json
import os
import shutil
from datetime import UTC, datetime
from pathlib import Path
from typing import Any

from automation.pi_symphony.board_policy import build_task_metadata
from automation.pi_symphony.config import LoadedWorkflow, WorkflowConfig
from automation.pi_symphony.github import (
    Issue,
    add_labels,
    comment_issue,
    create_pr,
    ensure_labels,
    find_pr_for_branch,
    list_ai_prs,
    list_ready_issues,
    remove_labels,
    view_issue,
)
from automation.pi_symphony.process import run, shell_join
from automation.pi_symphony.state import get_issue, logs_dir, update_issue
from automation.pi_symphony.workspace import branch_name, diff_stats, ensure_worktree, git_status_short, worktree_path


class GateFailure(RuntimeError):
    """A deterministic production gate failed."""


def utc_now() -> str:
    return datetime.now(UTC).isoformat(timespec="seconds")


def ensure_runtime_prereqs(config: WorkflowConfig) -> list[str]:
    missing: list[str] = []
    for command in ("git", "gh", "hermes", config.pi.command):
        if shutil.which(command) is None:
            missing.append(command)
    if config.autoreview.enabled and shutil.which(config.autoreview.command) is None:
        # A repo/profile-local explicit path is also accepted.
        if not Path(config.autoreview.command).expanduser().exists():
            missing.append(config.autoreview.command)
    return missing


def ensure_board(config: WorkflowConfig, repo_root: Path) -> str:
    slug = config.kanban.board_slug
    result = run(["hermes", "kanban", "boards", "show"], timeout=60, check=False)
    # `boards create` is idempotent enough for our purposes when guarded by list.
    boards = run(["hermes", "kanban", "boards", "list"], timeout=60, check=True).stdout
    if slug not in boards:
        run(
            [
                "hermes",
                "kanban",
                "boards",
                "create",
                slug,
                "--name",
                "pyqtgraph_to_cpp",
                "--description",
                "Pi Symphony automation board for michelreifenrath/pyqtgraph_to_cpp",
                "--default-workdir",
                str(repo_root),
            ],
            timeout=120,
        )
    if result.returncode != 0 or slug not in result.stdout:
        run(["hermes", "kanban", "boards", "switch", slug], timeout=60, check=False)
    return slug


def intake(loaded: LoadedWorkflow, *, limit: int | None = None, dry_run: bool = False) -> dict[str, Any]:
    config = loaded.config
    repo_root = loaded.repo_root
    missing = ensure_runtime_prereqs(config)
    # autoreview can be installed into the reviewer profile after the repo code is committed; intake itself can continue.
    blocking_missing = [cmd for cmd in missing if cmd != config.autoreview.command]
    if blocking_missing:
        raise GateFailure(f"missing required commands: {', '.join(blocking_missing)}")

    created_labels = [] if dry_run else ensure_labels(config)
    if not dry_run:
        ensure_board(config, repo_root)
    issues = list_ready_issues(config, limit=limit)
    actions: list[dict[str, Any]] = []
    for issue in issues:
        state = get_issue(repo_root, issue.number)
        if state.get("status") in {"claimed", "implemented", "reviewed", "released", "done"}:
            continue
        if dry_run:
            actions.append({"issue": issue.number, "action": "would_claim"})
            continue
        task_ids = create_issue_task_graph(config, repo_root, issue)
        update_issue(
            repo_root,
            issue.number,
            lambda item, issue=issue, task_ids=task_ids: item.update(
                {
                    "status": "claimed",
                    "title": issue.title,
                    "url": issue.url,
                    "labels": issue.labels,
                    "task_ids": task_ids,
                    "branch": branch_name(issue.number, issue.title),
                    "worktree": str(worktree_path(config, issue.number)),
                    "claimed_at": utc_now(),
                }
            ),
        )
        add_labels(config, issue.number, [config.github.claimed_label])
        remove_labels(config, issue.number, [config.github.ready_label])
        comment_issue(
            config,
            issue.number,
            "Pi Symphony automation has claimed this issue. A Hermes Kanban task graph was created: "
            + ", ".join(f"{phase}={task_id}" for phase, task_id in task_ids.items())
            + ". The system will implement in an isolated git worktree, run independent review/autoreview gates, then open a PR without auto-merge.",
        )
        actions.append({"issue": issue.number, "action": "claimed", "tasks": task_ids})
    return {"created_labels": created_labels, "actions": actions}


def create_issue_task_graph(config: WorkflowConfig, repo_root: Path, issue: Issue) -> dict[str, str]:
    metadata = build_task_metadata(issue.number, issue.labels, config.kanban, config.tracker.repo)
    tenant = str(metadata["tenant"])
    common = {
        "repo": config.tracker.repo,
        "issue": issue.number,
        "tenant": tenant,
        "tags": metadata["tags"],
        "branch": branch_name(issue.number, issue.title),
        "workflow": "pi-symphony-v1",
    }
    implement = _kanban_create(
        config,
        repo_root,
        title=f"issue #{issue.number}: implement {issue.title}",
        assignee="pi-worker",
        tenant=tenant,
        body=_task_body(config, issue, "implement", common),
        idempotency_key=f"{config.tracker.repo}#{issue.number}:implement",
        metadata=common | {"phase": "implement"},
    )
    review = _kanban_create(
        config,
        repo_root,
        title=f"issue #{issue.number}: review {issue.title}",
        assignee="pi-reviewer",
        tenant=tenant,
        body=_task_body(config, issue, "review", common),
        idempotency_key=f"{config.tracker.repo}#{issue.number}:review",
        parents=[implement],
        metadata=common | {"phase": "review"},
    )
    release = _kanban_create(
        config,
        repo_root,
        title=f"issue #{issue.number}: release PR {issue.title}",
        assignee="pi-release-manager",
        tenant=tenant,
        body=_task_body(config, issue, "release", common),
        idempotency_key=f"{config.tracker.repo}#{issue.number}:release",
        parents=[review],
        metadata=common | {"phase": "release"},
    )
    return {"implement": implement, "review": review, "release": release}


def _task_body(config: WorkflowConfig, issue: Issue, phase: str, common: dict[str, Any]) -> str:
    command = [
        "python3",
        "-m",
        "automation.pi_symphony.cli",
        "run-issue",
        "--workflow",
        "WORKFLOW.md",
        "--issue",
        str(issue.number),
        "--phase",
        phase,
        "--complete-current-task",
    ]
    return f"""GitHub issue: #{issue.number} {issue.title}
URL: {issue.url}
Repository: {config.tracker.repo}
Branch: {common['branch']}
Tenant: {common['tenant']}
Tags: {', '.join(common.get('tags') or []) or '(none)'}

Run this exact command from the repository root and let it perform the deterministic gate for this phase:

    {shell_join(command)}

Rules:
- Do not edit the main checkout directly.
- Do not push to main.
- Do not merge PRs.
- Treat Pi/autoreview/Codex output as advisory; deterministic gates in the command are authoritative.
- If the command blocks, leave the issue/task blocked with the printed reason.

Issue body:
{issue.body or '(no body)'}
"""


def _kanban_create(
    config: WorkflowConfig,
    repo_root: Path,
    *,
    title: str,
    body: str,
    assignee: str,
    tenant: str,
    idempotency_key: str,
    metadata: dict[str, Any],
    parents: list[str] | None = None,
) -> str:
    cmd = [
        "hermes",
        "kanban",
        "--board",
        config.kanban.board_slug,
        "create",
        title,
        "--body",
        body,
        "--assignee",
        assignee,
        "--workspace",
        f"dir:{repo_root}",
        "--tenant",
        tenant,
        "--idempotency-key",
        idempotency_key,
        "--created-by",
        "pi-symphony",
        "--max-retries",
        "1",
        "--json",
    ]
    for parent in parents or []:
        cmd.extend(["--parent", parent])
    # Kanban metadata is available at completion time rather than creation time in current Hermes CLI.
    result = run(cmd, timeout=120)
    data = json.loads(result.stdout)
    task_id = data.get("task_id") or data.get("id") or data.get("task", {}).get("id")
    if not task_id:
        raise RuntimeError(f"kanban create did not return a task id: {result.stdout}")
    return str(task_id)


def reconcile(loaded: LoadedWorkflow, *, dispatch: bool = True, dry_run: bool = False) -> dict[str, Any]:
    intake_result = intake(loaded, dry_run=dry_run)
    pr_result = {} if dry_run else check_prs(loaded.config, loaded.repo_root)
    dispatch_result = "dry-run"
    if dispatch and not dry_run:
        cmd = ["hermes", "kanban", "--board", loaded.config.kanban.board_slug, "dispatch", "--max", str(loaded.config.agent.max_concurrent_issues)]
        dispatch_result = run(cmd, timeout=180, check=False).combined_output
    return {"intake": intake_result, "prs": pr_result, "dispatch": dispatch_result}


def run_issue_phase(loaded: LoadedWorkflow, *, issue_number: int, phase: str, complete_current_task: bool = False) -> dict[str, Any]:
    try:
        if phase == "implement":
            result = implement_issue(loaded, issue_number)
        elif phase == "review":
            result = review_issue(loaded, issue_number)
        elif phase == "release":
            result = release_issue(loaded, issue_number)
        elif phase == "all":
            result = implement_issue(loaded, issue_number)
            result = review_issue(loaded, issue_number)
            result = release_issue(loaded, issue_number)
        else:
            raise GateFailure(f"unknown phase: {phase}")
    except Exception as exc:
        if complete_current_task:
            _block_current_task(loaded.config, f"{phase} failed for issue #{issue_number}: {exc}")
        raise
    if complete_current_task:
        _complete_current_task(loaded.config, f"{phase} complete for issue #{issue_number}", result)
    return result


def implement_issue(loaded: LoadedWorkflow, issue_number: int) -> dict[str, Any]:
    config = loaded.config
    issue = view_issue(config, issue_number)
    worktree, branch = ensure_worktree(config, loaded.repo_root, issue.number, issue.title)
    issue_log_dir = logs_dir(loaded.repo_root, issue.number)
    issue_log_dir.mkdir(parents=True, exist_ok=True)

    update_issue(
        loaded.repo_root,
        issue.number,
        lambda item: item.update({"status": "implementing", "attempts": int(item.get("attempts", 0)) + 1, "branch": branch, "worktree": str(worktree), "implementation_started_at": utc_now()}),
    )
    prompt = _pi_prompt(config, issue, branch)
    (issue_log_dir / "pi-prompt.md").write_text(prompt, encoding="utf-8")
    pi_cmd = _pi_command(config, thinking=config.pi.implementation_thinking, prompt=prompt)
    pi_result = run(pi_cmd, cwd=worktree, timeout=3600, check=False)
    (issue_log_dir / "pi-output.md").write_text(pi_result.combined_output + "\n", encoding="utf-8")
    if pi_result.returncode != 0:
        _mark_issue_failed(config, loaded.repo_root, issue.number, f"Pi implementation failed with exit code {pi_result.returncode}")
        raise GateFailure(f"Pi implementation failed; see {issue_log_dir / 'pi-output.md'}")

    status = git_status_short(worktree)
    if not status:
        raise GateFailure("Pi completed but left no git changes")
    validations = run_validations(config, worktree, issue_log_dir)
    review_commit = _commit_worktree_changes_for_review(worktree, issue_number=issue.number)
    changed_files, changed_lines = diff_stats(worktree, config.autoreview.base)
    if not changed_files:
        raise GateFailure("review diff is empty after implementation commit; refusing to attest an empty branch diff")
    summary = {
        "status": "implemented",
        "issue": issue.number,
        "branch": branch,
        "worktree": str(worktree),
        "review_commit": review_commit,
        "changed_files": changed_files,
        "changed_lines": changed_lines,
        "validations": validations,
        "completed_at": utc_now(),
    }
    (issue_log_dir / "implementation.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    update_issue(loaded.repo_root, issue.number, lambda item: item.update(summary))
    return summary


def _pi_command(config: WorkflowConfig, *, thinking: str, prompt: str) -> list[str]:
    cmd = [config.pi.command]
    if config.pi.provider:
        cmd.extend(["--provider", config.pi.provider])
    if config.pi.model:
        cmd.extend(["--model", config.pi.model])
    cmd.extend(["--thinking", thinking, "--print", "--no-session", prompt])
    return cmd


def _commit_worktree_changes_for_review(worktree: Path, *, issue_number: int) -> str | None:
    """Create the local review commit that branch-diff autoreview will inspect."""
    if not git_status_short(worktree):
        return None
    run(["git", "add", "-A"], cwd=worktree, timeout=120)
    commit_message = (
        f"fix: address issue #{issue_number}\n\n"
        "Automated Pi Symphony implementation.\n\n"
        "Create a local review commit before autoreview so branch diffs include newly created files."
    )
    commit_result = run(["git", "commit", "-m", commit_message], cwd=worktree, timeout=300, check=False)
    if commit_result.returncode != 0:
        if "nothing to commit" in commit_result.combined_output.lower():
            return None
        raise GateFailure(f"git commit failed before review: {commit_result.combined_output}")
    return run(["git", "rev-parse", "HEAD"], cwd=worktree, timeout=60).stdout.strip()


def review_issue(loaded: LoadedWorkflow, issue_number: int) -> dict[str, Any]:
    config = loaded.config
    issue = view_issue(config, issue_number)
    worktree = worktree_path(config, issue.number)
    if not worktree.exists():
        raise GateFailure(f"missing worktree: {worktree}")
    issue_log_dir = logs_dir(loaded.repo_root, issue.number)
    issue_log_dir.mkdir(parents=True, exist_ok=True)
    validations = run_validations(config, worktree, issue_log_dir)
    changed_files, changed_lines = diff_stats(worktree, config.autoreview.base)
    if not changed_files:
        raise GateFailure("review diff is empty; refusing to run autoreview without branch changes")
    if len(changed_files) > config.policy.max_changed_files_without_human_review or changed_lines > config.policy.max_diff_lines_without_human_review:
        add_labels(config, issue.number, [config.github.human_review_label, config.github.blocked_label])
        raise GateFailure(
            f"diff too large for autonomous release: {len(changed_files)} files, {changed_lines} changed lines"
        )
    review = run_autoreview(config, worktree, issue_log_dir)
    reviewed_head = run(["git", "rev-parse", "HEAD"], cwd=worktree, timeout=60).stdout.strip()
    summary = {
        "status": "reviewed",
        "issue": issue.number,
        "changed_files": changed_files,
        "changed_lines": changed_lines,
        "validations": validations,
        "autoreview": review,
        "reviewed_head": reviewed_head,
        "reviewed_at": utc_now(),
    }
    (issue_log_dir / "review.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    update_issue(loaded.repo_root, issue.number, lambda item: item.update(summary))
    return summary


def release_issue(loaded: LoadedWorkflow, issue_number: int) -> dict[str, Any]:
    config = loaded.config
    issue = view_issue(config, issue_number)
    state = get_issue(loaded.repo_root, issue.number)
    if config.policy.require_independent_review and state.get("status") != "reviewed":
        raise GateFailure("release requires the review phase to complete first")
    worktree = worktree_path(config, issue.number)
    if not worktree.exists():
        raise GateFailure(f"missing worktree: {worktree}")
    branch = str(state.get("branch") or branch_name(issue.number, issue.title))
    if branch in {config.workspace.base_branch, "main", "master"}:
        raise GateFailure(f"refusing to release protected branch {branch!r}")
    issue_log_dir = logs_dir(loaded.repo_root, issue.number)
    validations = run_validations(config, worktree, issue_log_dir)
    status = git_status_short(worktree)
    if status:
        raise GateFailure(f"release found uncommitted changes after review; refusing to commit unreviewed changes:\n{status}")
    reviewed_head = str(state.get("reviewed_head") or state.get("review_commit") or "")
    if not reviewed_head:
        raise GateFailure("release requires a recorded reviewed_head from the review phase")
    current_head = run(["git", "rev-parse", "HEAD"], cwd=worktree, timeout=60).stdout.strip()
    if current_head != reviewed_head:
        raise GateFailure(f"release HEAD {current_head} does not match reviewed head {reviewed_head}; rerun review before release")
    run(["git", "push", "-u", "origin", branch], cwd=worktree, timeout=300)
    existing_pr = find_pr_for_branch(config, branch)
    if existing_pr is None:
        pr_body = _pr_body(issue, state, validations)
        pr = create_pr(config, branch=branch, title=f"fix: address issue #{issue.number} - {issue.title}", body=pr_body)
    else:
        pr = existing_pr
    if pr.get("number"):
        add_labels(config, int(pr["number"]), [config.github.merge_ready_label])
    add_labels(config, issue.number, [config.github.review_label])
    remove_labels(config, issue.number, [config.github.claimed_label, config.github.failed_label, config.github.blocked_label])
    comment_issue(config, issue.number, f"Pi Symphony opened/updated PR: {pr.get('url')}. Auto-merge is disabled; please review manually.")
    summary = {"status": "released", "issue": issue.number, "branch": branch, "pr": pr, "validations": validations, "released_at": utc_now()}
    (issue_log_dir / "release.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    update_issue(loaded.repo_root, issue.number, lambda item: item.update(summary))
    return summary


def run_validations(config: WorkflowConfig, worktree: Path, issue_log_dir: Path) -> list[dict[str, Any]]:
    results: list[dict[str, Any]] = []
    if config.validation.diff_check:
        result = run(["git", "diff", "--check"], cwd=worktree, timeout=120, check=False)
        record = {"command": "git diff --check", "returncode": result.returncode, "output": result.combined_output}
        results.append(record)
        if result.returncode != 0:
            raise GateFailure(f"git diff --check failed: {result.combined_output}")
    for index, command in enumerate(config.validation.commands):
        result = run(command, cwd=worktree, timeout=1800, check=False, shell=True)
        record = {"command": command, "returncode": result.returncode, "output": result.combined_output[-8000:]}
        results.append(record)
        (issue_log_dir / f"validation-{index}.log").write_text(result.combined_output + "\n", encoding="utf-8")
        if result.returncode != 0:
            raise GateFailure(f"validation failed: {command}; see {issue_log_dir / f'validation-{index}.log'}")
    return results


def run_autoreview(config: WorkflowConfig, worktree: Path, issue_log_dir: Path) -> dict[str, Any]:
    command_path = shutil.which(config.autoreview.command) or str(Path(config.autoreview.command).expanduser())
    if command_path and Path(command_path).exists() and Path(command_path).name == "autoreview":
        json_output = issue_log_dir / "autoreview.json"
        prompt_file = issue_log_dir / "review-context.md"
        prompt_file.write_text("Review this branch diff for correctness, regressions, tests, and security. Return actionable findings only.\n", encoding="utf-8")
        cmd = [command_path, "--mode", config.autoreview.mode, "--base", config.autoreview.base, "--prompt-file", str(prompt_file), "--json-output", str(json_output)]
        result = run(cmd, cwd=worktree, timeout=1800, check=False)
        (issue_log_dir / "autoreview.log").write_text(result.combined_output + "\n", encoding="utf-8")
        if result.returncode != 0:
            raise GateFailure(f"autoreview failed: {result.combined_output[-2000:]}")
        return {"engine": "autoreview", "command": shell_join(cmd), "log": str(issue_log_dir / "autoreview.log"), "json": str(json_output)}

    if shutil.which("codex"):
        cmd = ["codex", "review", "--base", config.autoreview.base]
        result = run(cmd, cwd=worktree, timeout=1800, check=False)
        (issue_log_dir / "codex-review.log").write_text(result.combined_output + "\n", encoding="utf-8")
        if result.returncode != 0:
            raise GateFailure(f"codex review failed and autoreview is unavailable: {result.combined_output[-2000:]}")
        return {"engine": "codex-review", "command": shell_join(cmd), "log": str(issue_log_dir / "codex-review.log")}
    raise GateFailure("neither autoreview nor codex review is available")


def check_prs(config: WorkflowConfig, repo_root: Path) -> dict[str, Any]:
    updated: list[dict[str, Any]] = []
    for pr in list_ai_prs(config):
        if pr.get("state") != "MERGED":
            continue
        head = str(pr.get("headRefName") or "")
        issue = _issue_number_from_branch(head)
        if issue is None:
            continue
        add_labels(config, issue, [config.github.done_label])
        remove_labels(config, issue, [config.github.claimed_label, config.github.review_label, config.github.failed_label, config.github.blocked_label])
        update_issue(repo_root, issue, lambda item, pr=pr: item.update({"status": "done", "merged_pr": pr, "done_at": utc_now()}))
        updated.append({"issue": issue, "pr": pr.get("number"), "state": "done"})
    return {"updated": updated}


def _issue_number_from_branch(branch: str) -> int | None:
    prefix = "ai/issue-"
    if not branch.startswith(prefix):
        return None
    rest = branch[len(prefix) :].split("-", 1)[0]
    return int(rest) if rest.isdigit() else None


def _pi_prompt(config: WorkflowConfig, issue: Issue, branch: str) -> str:
    return f"""Use pi subagents for this implementation: first have a scout inspect the repo, then a planner make a file-level plan, then an implementer change code, then a tester run checks. Do not commit, push, or merge. Leave a clean git diff in the current worktree.

Repository: {config.tracker.repo}
Branch: {branch}
Issue: #{issue.number} {issue.title}
Author: {issue.author}
URL: {issue.url}
Labels: {', '.join(issue.labels) or '(none)'}

Issue body:
{issue.body or '(no body)'}

Repo-owned workflow:
{config.body}

Acceptance rules:
- Implement only the issue scope.
- Add or update tests for behavior changes.
- Run the relevant checks before finalizing.
- Do not leave scratch artifacts such as .pi-lens, temp files, or debug logs in the diff.
- Do not modify WORKFLOW.md or automation policy files unless the issue explicitly asks for it.
"""


def _pr_body(issue: Issue, state: dict[str, Any], validations: list[dict[str, Any]]) -> str:
    validation_lines = "\n".join(
        f"- `{item['command']}`: {'PASS' if item['returncode'] == 0 else 'FAIL'}" for item in validations
    )
    changed = state.get("changed_files") or []
    changed_lines = "\n".join(f"- `{path}`" for path in changed) or "- See diff"
    return f"""## Summary
Automated Pi Symphony implementation for #{issue.number}.

## Changed files
{changed_lines}

## Validation
{validation_lines or '- No validation commands configured'}

## Safety
- Implemented in an isolated git worktree.
- Independent review/autoreview gate completed before PR creation.
- Auto-merge is disabled by policy.

Closes #{issue.number}
"""


def _mark_issue_failed(config: WorkflowConfig, repo_root: Path, issue_number: int, reason: str) -> None:
    add_labels(config, issue_number, [config.github.failed_label, config.github.blocked_label])
    update_issue(repo_root, issue_number, lambda item: item.update({"status": "failed", "failure": reason, "failed_at": utc_now()}))


def _complete_current_task(config: WorkflowConfig, summary: str, metadata: dict[str, Any]) -> None:
    task_id = os.environ.get("HERMES_KANBAN_TASK")
    if not task_id:
        return
    run(
        [
            "hermes",
            "kanban",
            "--board",
            os.environ.get("HERMES_KANBAN_BOARD", config.kanban.board_slug),
            "complete",
            task_id,
            "--summary",
            summary,
            "--metadata",
            json.dumps(metadata)[:12000],
        ],
        timeout=120,
        check=False,
    )


def _block_current_task(config: WorkflowConfig, reason: str) -> None:
    task_id = os.environ.get("HERMES_KANBAN_TASK")
    if not task_id:
        return
    run(
        [
            "hermes",
            "kanban",
            "--board",
            os.environ.get("HERMES_KANBAN_BOARD", config.kanban.board_slug),
            "block",
            task_id,
            reason[:1000],
        ],
        timeout=120,
        check=False,
    )
