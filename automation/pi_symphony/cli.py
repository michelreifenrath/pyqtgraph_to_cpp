from __future__ import annotations

import argparse
import json
import sys
from collections.abc import Sequence
from pathlib import Path

from automation.pi_symphony.board_policy import derive_tags, derive_tenant
from automation.pi_symphony.config import ConfigError, load_workflow, load_workflow_with_context
from automation.pi_symphony.runner import GateFailure, ensure_board, ensure_runtime_prereqs, intake, reconcile, run_issue_phase, check_prs


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    return args.func(args)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="pi-symphony")
    subparsers = parser.add_subparsers(dest="command", required=True)

    validate = subparsers.add_parser("validate-workflow", help="parse WORKFLOW.md and report validation status")
    validate.add_argument("--workflow", default="WORKFLOW.md", type=Path)
    validate.set_defaults(func=_validate_workflow)

    doctor = subparsers.add_parser("doctor", help="check runtime prerequisites and policy")
    doctor.add_argument("--workflow", default="WORKFLOW.md", type=Path)
    doctor.set_defaults(func=_doctor)

    board = subparsers.add_parser("board-policy", help="print board tenant/tag policy information")
    board.add_argument("--workflow", default="WORKFLOW.md", type=Path)
    board.add_argument("labels", nargs="*")
    board.set_defaults(func=_board_policy)

    setup = subparsers.add_parser("setup", help="idempotently create GitHub labels and Hermes Kanban board")
    setup.add_argument("--workflow", default="WORKFLOW.md", type=Path)
    setup.add_argument("--json", action="store_true")
    setup.set_defaults(func=_setup)

    intake_cmd = subparsers.add_parser("intake", help="claim ai:ready GitHub issues and create Kanban task graphs")
    intake_cmd.add_argument("--workflow", default="WORKFLOW.md", type=Path)
    intake_cmd.add_argument("--limit", type=int)
    intake_cmd.add_argument("--dry-run", action="store_true")
    intake_cmd.add_argument("--json", action="store_true")
    intake_cmd.set_defaults(func=_intake)

    reconcile_cmd = subparsers.add_parser("reconcile", help="run intake, PR reconciliation, and one Kanban dispatch pass")
    reconcile_cmd.add_argument("--workflow", default="WORKFLOW.md", type=Path)
    reconcile_cmd.add_argument("--dry-run", action="store_true")
    reconcile_cmd.add_argument("--no-dispatch", action="store_true")
    reconcile_cmd.add_argument("--quiet", action="store_true", help="print nothing when no action occurred")
    reconcile_cmd.add_argument("--json", action="store_true")
    reconcile_cmd.set_defaults(func=_reconcile)

    run_issue = subparsers.add_parser("run-issue", help="run a deterministic phase for one GitHub issue")
    run_issue.add_argument("--workflow", default="WORKFLOW.md", type=Path)
    run_issue.add_argument("--issue", required=True, type=int)
    run_issue.add_argument("--phase", choices=["implement", "review", "release", "all"], default="all")
    run_issue.add_argument("--complete-current-task", action="store_true")
    run_issue.add_argument("--json", action="store_true")
    run_issue.set_defaults(func=_run_issue)

    prs = subparsers.add_parser("check-prs", help="mark merged AI PRs/issues done")
    prs.add_argument("--workflow", default="WORKFLOW.md", type=Path)
    prs.add_argument("--json", action="store_true")
    prs.set_defaults(func=_check_prs)
    return parser


def _validate_workflow(args: argparse.Namespace) -> int:
    try:
        config = load_workflow(args.workflow)
    except (ConfigError, OSError, TypeError) as exc:
        print(f"workflow invalid: {exc}", file=sys.stderr)
        return 1
    print(f"workflow valid: {args.workflow}")
    print(f"tracker.repo: {config.tracker.repo}")
    print(f"kanban.board_slug: {config.kanban.board_slug}")
    print(f"workspace.root: {config.workspace.root}")
    print(f"github.ready_label: {config.github.ready_label}")
    return 0


def _doctor(args: argparse.Namespace) -> int:
    try:
        loaded = load_workflow_with_context(args.workflow)
    except (ConfigError, OSError, TypeError) as exc:
        print(f"workflow invalid: {exc}", file=sys.stderr)
        return 1
    missing = ensure_runtime_prereqs(loaded.config)
    if missing:
        print("missing runtime commands: " + ", ".join(missing), file=sys.stderr)
        return 1
    print("runtime prerequisites ok")
    return 0


def _setup(args: argparse.Namespace) -> int:
    try:
        loaded = load_workflow_with_context(args.workflow)
        from automation.pi_symphony.github import ensure_labels

        created_labels = ensure_labels(loaded.config)
        board = ensure_board(loaded.config, loaded.repo_root)
    except (ConfigError, OSError, RuntimeError, TypeError) as exc:
        print(f"setup failed: {exc}", file=sys.stderr)
        return 1
    result = {"created_labels": created_labels, "board": board}
    _print_result(result, args.json)
    return 0


def _board_policy(args: argparse.Namespace) -> int:
    try:
        config = load_workflow(args.workflow)
    except (ConfigError, OSError, TypeError) as exc:
        print(f"workflow invalid: {exc}", file=sys.stderr)
        return 1
    tags = derive_tags(args.labels, config.kanban)
    print(f"board_slug: {config.kanban.board_slug}")
    print(f"tenant: {derive_tenant(args.labels, config.kanban)}")
    print(f"tags: {', '.join(tags)}")
    print(f"source_repo: {config.tracker.repo}")
    return 0


def _intake(args: argparse.Namespace) -> int:
    try:
        result = intake(load_workflow_with_context(args.workflow), limit=args.limit, dry_run=args.dry_run)
    except (ConfigError, OSError, RuntimeError, TypeError, GateFailure) as exc:
        print(f"intake failed: {exc}", file=sys.stderr)
        return 1
    _print_result(result, args.json)
    return 0


def _reconcile(args: argparse.Namespace) -> int:
    try:
        result = reconcile(load_workflow_with_context(args.workflow), dispatch=not args.no_dispatch, dry_run=args.dry_run)
    except (ConfigError, OSError, RuntimeError, TypeError, GateFailure) as exc:
        print(f"reconcile failed: {exc}", file=sys.stderr)
        return 1
    if args.quiet and _no_reconcile_actions(result):
        return 0
    _print_result(result, args.json)
    return 0


def _run_issue(args: argparse.Namespace) -> int:
    try:
        result = run_issue_phase(
            load_workflow_with_context(args.workflow),
            issue_number=args.issue,
            phase=args.phase,
            complete_current_task=args.complete_current_task,
        )
    except (ConfigError, OSError, RuntimeError, TypeError, GateFailure) as exc:
        print(f"run-issue failed: {exc}", file=sys.stderr)
        return 1
    _print_result(result, args.json)
    return 0


def _check_prs(args: argparse.Namespace) -> int:
    try:
        loaded = load_workflow_with_context(args.workflow)
        result = check_prs(loaded.config, loaded.repo_root)
    except (ConfigError, OSError, RuntimeError, TypeError, GateFailure) as exc:
        print(f"check-prs failed: {exc}", file=sys.stderr)
        return 1
    _print_result(result, args.json)
    return 0


def _print_result(result: object, as_json: bool) -> None:
    if as_json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        if isinstance(result, dict):
            print(json.dumps(result, indent=2, sort_keys=True))
        else:
            print(result)


def _no_reconcile_actions(result: dict[str, object]) -> bool:
    intake_result = result.get("intake") if isinstance(result, dict) else None
    pr_result = result.get("prs") if isinstance(result, dict) else None
    no_intake = isinstance(intake_result, dict) and not intake_result.get("actions") and not intake_result.get("created_labels")
    no_prs = isinstance(pr_result, dict) and not pr_result.get("updated")
    dispatch = str(result.get("dispatch", "")).strip()
    no_dispatch = not dispatch or "No ready tasks" in dispatch or "dry-run" == dispatch
    return bool(no_intake and no_prs and no_dispatch)


if __name__ == "__main__":
    raise SystemExit(main())
