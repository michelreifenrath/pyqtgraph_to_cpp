"""Cron tick launcher for an advisory persistent Guardian agent.

This module is intentionally advisory-only.  It collects a compact snapshot of
workflow state, resumes the same Hermes Guardian session, and prints the
Guardian's response for cron delivery.  It does not register or start cron jobs
by itself.
"""

from __future__ import annotations

import argparse
import dataclasses
import fcntl
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable, Sequence

REPO_ROOT = Path(os.environ.get("PYQTGRAPH_CPP_REPO", "/home/michel/code/pyqtgraph_to_cpp"))
BOARD = os.environ.get("PYQTGRAPH_CPP_GUARDIAN_BOARD", "pyqtgraph-to-cpp")
GITHUB_REPO = os.environ.get("PYQTGRAPH_CPP_GITHUB_REPO", "michelreifenrath/pyqtgraph_to_cpp")
PROFILE = os.environ.get("PYQTGRAPH_CPP_GUARDIAN_PROFILE", "pyqtgraphguardian")
SESSION_NAME = os.environ.get("PYQTGRAPH_CPP_GUARDIAN_SESSION", "pyqtgraph-cpp-guardian")
SCRIPT_STATE_DIR = Path(os.environ.get("PYQTGRAPH_CPP_GUARDIAN_STATE_DIR", "/home/michel/.hermes/scripts/state"))
STATE_FILE = SCRIPT_STATE_DIR / "pyqtgraph_cpp_guardian_agent.json"
LOCK_FILE = SCRIPT_STATE_DIR / "pyqtgraph_cpp_guardian_agent.lock"
REPORT_DIR = Path(os.environ.get("PYQTGRAPH_CPP_GUARDIAN_REPORT_DIR", str(REPO_ROOT / ".hermes" / "guardian" / "reports")))
DEFAULT_TIMEOUT = 30
MAX_FIELD_CHARS = 12_000
MAX_COMMAND_CHARS = 24_000

SENSITIVE_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"(?i)((?:api[_-]?key|token|secret|password)\s*[=:]\s*)\S+"),
    re.compile(r"(?i)(authorization\s*:\s*bearer\s+)\S+"),
    re.compile(r"(?i)()(?:(?:ghp|github_pat|sk-[A-Za-z0-9])[A-Za-z0-9_\-]{12,})"),
)

PROCESS_PATTERNS = (
    "pyqtgraph_cpp_autorun",
    "pyqtgraph_cpp_guardian",
    "hermes kanban",
    "automation.pi_symphony",
    "pi --",
    "/bin/pi",
    "codex",
    "autoreview",
)


@dataclasses.dataclass(frozen=True)
class CommandResult:
    command: str
    exit_code: int | None
    output: str
    timed_out: bool = False


def utc_now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def redact(text: str) -> str:
    redacted = text
    for pattern in SENSITIVE_PATTERNS:
        redacted = pattern.sub(lambda m: f"{m.group(1)}[REDACTED]", redacted)
    return redacted


def truncate(text: str, limit: int = MAX_FIELD_CHARS) -> str:
    if len(text) <= limit:
        return text
    return text[:limit] + f"\n[truncated {len(text) - limit} chars]"


def subprocess_text(value: object) -> str:
    if isinstance(value, str):
        return value
    if isinstance(value, bytes):
        return value.decode("utf-8", errors="replace")
    return ""


def run_command(args: Sequence[str], *, cwd: Path = REPO_ROOT, timeout: int = DEFAULT_TIMEOUT) -> CommandResult:
    display = " ".join(args)
    try:
        completed = subprocess.run(
            list(args),
            cwd=str(cwd),
            text=True,
            capture_output=True,
            timeout=timeout,
            check=False,
            env=command_env(),
        )
        output = redact((completed.stdout or "") + (completed.stderr or ""))
        return CommandResult(display, completed.returncode, truncate(output, MAX_COMMAND_CHARS))
    except subprocess.TimeoutExpired as exc:
        output = redact(subprocess_text(exc.stdout) + subprocess_text(exc.stderr))
        return CommandResult(display, None, truncate(output, MAX_COMMAND_CHARS), timed_out=True)
    except OSError as exc:
        return CommandResult(display, None, f"command failed to start: {exc}")


def command_env() -> dict[str, str]:
    env = os.environ.copy()
    path_parts = [
        "/home/michel/.local/bin",
        "/home/michel/.cargo/bin",
        "/usr/local/bin",
        "/usr/bin",
        "/bin",
    ]
    existing = env.get("PATH", "")
    env["PATH"] = ":".join(path_parts + ([existing] if existing else []))
    env.setdefault("NO_COLOR", "1")
    return env


def command_block(result: CommandResult) -> str:
    status = "timeout" if result.timed_out else result.exit_code
    body = result.output.strip() or "<no output>"
    return f"$ {result.command}\nexit={status}\n{body}"


def matching_processes() -> str:
    ps = run_command(["ps", "-eo", "pid,ppid,pgid,stat,etime,cmd"], cwd=Path("/"), timeout=15)
    if ps.exit_code not in (0, None):
        return command_block(ps)
    lines = []
    for line in ps.output.splitlines():
        if "guardian_agent_tick.py" in line or "pyqtgraph_cpp_guardian_agent_tick.py" in line:
            continue
        if any(pattern in line for pattern in PROCESS_PATTERNS):
            lines.append(line)
    return "\n".join(lines) if lines else "<no matching processes>"


def safe_json_summary(raw: str, *, keys: Iterable[str]) -> str:
    try:
        data = json.loads(raw)
    except json.JSONDecodeError:
        return truncate(raw, 6000)
    if not isinstance(data, list):
        return truncate(json.dumps(data, indent=2, sort_keys=True), 6000)
    wanted = tuple(keys)
    compact: list[dict[str, object]] = []
    for item in data[:30]:
        if not isinstance(item, dict):
            compact.append({"value": item})
            continue
        compact.append({key: item.get(key) for key in wanted if key in item})
    if len(data) > 30:
        compact.append({"truncated_count": len(data) - 30})
    return truncate(json.dumps(compact, indent=2, sort_keys=True), 8000)


def collect_snapshot() -> dict[str, str]:
    snapshot: dict[str, str] = {
        "timestamp_utc": utc_now(),
        "repo_root": str(REPO_ROOT),
        "board": BOARD,
        "github_repo": GITHUB_REPO,
        "profile": PROFILE,
        "session_name": SESSION_NAME,
        "mode": "test-phase advisory-only; always report every tick; 10 minute intended cadence",
    }

    commands = {
        "git_status": (["git", "status", "--short", "--branch"], 20),
        "git_branch": (["git", "branch", "--show-current"], 10),
        "git_ahead_behind": (["git", "rev-list", "--left-right", "--count", "HEAD...@{upstream}"], 20),
        "workflow_validate": ([sys.executable, "-m", "automation.pi_symphony.cli", "validate-workflow", "--workflow", "WORKFLOW.md"], 60),
        "kanban_stats": (["hermes", "kanban", "--board", BOARD, "stats"], 60),
        "kanban_list_running": (["hermes", "kanban", "--board", BOARD, "list", "--status", "running", "--json"], 60),
        "kanban_list_blocked": (["hermes", "kanban", "--board", BOARD, "list", "--status", "blocked", "--json"], 60),
    }
    for key, (cmd, timeout) in commands.items():
        snapshot[key] = command_block(run_command(cmd, timeout=timeout))

    snapshot["matching_processes"] = matching_processes()

    issues = run_command(
        [
            "gh",
            "api",
            "-X",
            "GET",
            f"repos/{GITHUB_REPO}/issues",
            "-f",
            "state=open",
            "-f",
            "per_page=30",
        ],
        timeout=60,
    )
    snapshot["github_open_issues"] = command_block(
        dataclasses.replace(
            issues,
            output=safe_json_summary(issues.output, keys=("number", "title", "state", "labels", "pull_request", "html_url")),
        )
    )

    prs = run_command(
        [
            "gh",
            "pr",
            "list",
            "--repo",
            GITHUB_REPO,
            "--state",
            "open",
            "--json",
            "number,title,isDraft,mergeStateStatus,headRefName,headRefOid,url,reviewDecision,statusCheckRollup",
            "--limit",
            "20",
        ],
        timeout=60,
    )
    snapshot["github_open_prs"] = command_block(
        dataclasses.replace(
            prs,
            output=safe_json_summary(
                prs.output,
                keys=("number", "title", "isDraft", "mergeStateStatus", "headRefName", "headRefOid", "reviewDecision", "url"),
            ),
        )
    )

    snapshot["recent_pi_symphony_logs"] = recent_logs()
    return snapshot


def recent_logs() -> str:
    logs_root = REPO_ROOT / ".hermes" / "pi-symphony" / "logs"
    if not logs_root.exists():
        return "<logs directory missing>"
    files = sorted((p for p in logs_root.rglob("*") if p.is_file()), key=lambda p: p.stat().st_mtime, reverse=True)[:12]
    entries: list[str] = []
    for path in files:
        rel = path.relative_to(REPO_ROOT)
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError as exc:
            entries.append(f"## {rel}\n<read failed: {exc}>")
            continue
        tail = "\n".join(text.splitlines()[-40:])
        entries.append(f"## {rel}\n{redact(tail)}")
    return truncate("\n\n".join(entries) if entries else "<no log files>", 18_000)


def load_state() -> dict[str, object]:
    try:
        return json.loads(STATE_FILE.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}


def save_state(state: dict[str, object]) -> None:
    SCRIPT_STATE_DIR.mkdir(parents=True, exist_ok=True)
    tmp = STATE_FILE.with_suffix(".tmp")
    tmp.write_text(json.dumps(state, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    tmp.replace(STATE_FILE)


def fingerprint(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8", errors="replace")).hexdigest()[:16]


def report_timestamp_slug(timestamp_utc: str) -> str:
    try:
        parsed = datetime.fromisoformat(timestamp_utc.replace("Z", "+00:00"))
    except ValueError:
        parsed = datetime.now(timezone.utc)
    parsed = parsed.astimezone(timezone.utc).replace(microsecond=0)
    return parsed.strftime("%Y%m%dT%H%M%SZ")


def save_guardian_report(
    *,
    snapshot: dict[str, str],
    output: str,
    result: CommandResult,
    state: dict[str, object],
) -> Path:
    timestamp = snapshot.get("timestamp_utc") or utc_now()
    slug = report_timestamp_slug(timestamp)
    report_dir = REPORT_DIR / slug[0:4] / slug[4:6] / slug[6:8]
    report_dir.mkdir(parents=True, exist_ok=True)
    path = report_dir / f"{slug}-guardian-report.md"

    snapshot_json = json.dumps(snapshot, indent=2, sort_keys=True)
    state_json = json.dumps(state, indent=2, sort_keys=True)
    status = "timeout" if result.timed_out else result.exit_code
    content = f"""# Guardian report: pyqtgraph_to_cpp

- timestamp_utc: `{timestamp}`
- health_source: `pyqtgraphguardian`
- persistent_session: `{SESSION_NAME}`
- command_exit: `{status}`
- mode: `test-phase advisory-only`

## Guardian output

```text
{redact(output.strip() or '<no output>')}
```

## Snapshot

```json
{redact(snapshot_json)}
```

## Script state before tick

```json
{redact(state_json)}
```
"""
    path.write_text(content, encoding="utf-8")

    latest = {
        "timestamp_utc": timestamp,
        "report_path": str(path),
        "exit_code": result.exit_code,
        "timed_out": result.timed_out,
        "output_hash": fingerprint(output),
        "snapshot_hash": fingerprint(json.dumps(snapshot, sort_keys=True)),
    }
    (REPORT_DIR / "latest.json").write_text(json.dumps(latest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return path


def build_prompt(snapshot: dict[str, str], state: dict[str, object]) -> str:
    snapshot_json = json.dumps(snapshot, indent=2, sort_keys=True)
    state_json = json.dumps(state, indent=2, sort_keys=True)
    return f"""You are the persistent advisory Guardian agent for Michel's pyqtgraph_to_cpp workflow.

You are resumed under the same Hermes session every tick. Use your prior session history plus the fresh snapshot below.

TEST-PHASE POLICY:
- Always produce a compact Telegram-suitable status for this tick.
- Be critical: look for flaws, unsafe states, duplicate workers, stale cards, broken gates, and workflow improvements.
- Advisory only: do not mutate state, do not restart/kill/dispatch/archive/edit/rebase/push/merge, and do not create cron jobs.
- If Michel needs to decide, ask exactly one clear question and include your recommended default.
- Distinguish mechanical health from workflow outcome.
- Do not expose secrets; write [REDACTED] if any appear.

THREE-LAYER ARCHITECTURE:
- Worker layer: pi-worker/pi-release-manager execute implementation, rework, and release preparation. Do not let worker self-approval stand in for review.
- Supervisor layer: pi-reviewer, deterministic tests, workflow validation, autoreview proof, release gates, and PR safety checks evaluate Worker output before publication or execution.
- Meta layer: you are the Meta observer. You analyze repeated failures, missing context, weak prompts, stale-state patterns, and absent deterministic guardrails. You may propose improvements, but you must not silently change prompts, code, cards, labels, or cron jobs.

CONTINUOUS LEARNING RULE:
- When you identify a real incident or repeated failure mode, you must propose Synthetic regression candidates unless the issue is obviously one-off external infrastructure.
- Each candidate should say: incident, likely root cause, missing guardrail/test, proposed test fixture or assertion, and whether a prompt/workflow/doc patch is needed.
- Meta-layer updates are proposals only; they need deterministic validation plus reviewer or Michel approval before application.

Output format:
Guardian tick: pyqtgraph_to_cpp
Health: OK | Warning | Critical
Workflow: progressing | idle | blocked | unknown
Findings:
- ...
Meta-learning candidates:
- none | <incident -> root cause -> proposed regression/guardrail>
Action for Michel: none | <one question/recommendation>

Script-side prior state:
```json
{state_json}
```

Fresh snapshot:
```json
{snapshot_json}
```
"""


def hermes_executable() -> str:
    return shutil.which("hermes", path=command_env().get("PATH")) or "hermes"


def invoke_guardian(prompt: str, *, timeout: int = 300) -> CommandResult:
    return run_command(
        [
            hermes_executable(),
            "-p",
            PROFILE,
            "--continue",
            SESSION_NAME,
            "chat",
            "-Q",
            "--source",
            "cron:pyqtgraph-guardian",
            "--toolsets",
            "safe",
            "-q",
            prompt,
        ],
        cwd=REPO_ROOT,
        timeout=timeout,
    )


def acquire_lock():
    SCRIPT_STATE_DIR.mkdir(parents=True, exist_ok=True)
    handle = LOCK_FILE.open("w", encoding="utf-8")
    try:
        fcntl.flock(handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        handle.close()
        return None
    handle.write(f"pid={os.getpid()} timestamp={utc_now()}\n")
    handle.flush()
    return handle


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Run one advisory pyqtgraph_to_cpp Guardian agent tick.")
    parser.add_argument("--dry-run", action="store_true", help="Collect snapshot and print the prompt, but do not invoke Hermes.")
    parser.add_argument("--json-snapshot", action="store_true", help="Collect snapshot and print JSON, but do not invoke Hermes.")
    parser.add_argument("--guardian-timeout", type=int, default=300, help="Seconds to wait for the Guardian Hermes invocation.")
    args = parser.parse_args(argv)

    lock = acquire_lock()
    if lock is None:
        print("Guardian tick skipped: previous pyqtgraph_to_cpp Guardian tick is still running.")
        return 0

    try:
        state = load_state()
        snapshot = collect_snapshot()
        prompt = build_prompt(snapshot, state)

        if args.json_snapshot:
            print(json.dumps(snapshot, indent=2, sort_keys=True))
            return 0
        if args.dry_run:
            print(prompt)
            return 0

        result = invoke_guardian(prompt, timeout=args.guardian_timeout)
        output = result.output.strip()
        report_path = save_guardian_report(snapshot=snapshot, output=output, result=result, state=state)
        new_state = dict(state)
        new_state.update(
            {
                "last_tick_utc": snapshot["timestamp_utc"],
                "last_snapshot_hash": fingerprint(json.dumps(snapshot, sort_keys=True)),
                "last_guardian_exit_code": result.exit_code,
                "last_guardian_timed_out": result.timed_out,
                "last_output_hash": fingerprint(output),
                "last_report_path": str(report_path),
            }
        )
        if result.exit_code == 0 and not result.timed_out:
            new_state["consecutive_failures"] = 0
        else:
            prior_failures = new_state.get("consecutive_failures", 0)
            if not isinstance(prior_failures, int):
                prior_failures = 0
            new_state["consecutive_failures"] = prior_failures + 1
        save_state(new_state)

        if result.exit_code != 0 or result.timed_out:
            print("Guardian tick failed while invoking persistent Guardian agent.")
            print(command_block(result))
            print(f"Local report: {report_path}")
            return 2

        print(output or "Guardian tick completed but produced no output.")
        print(f"Local report: {report_path}")
        return 0
    finally:
        try:
            fcntl.flock(lock, fcntl.LOCK_UN)
        finally:
            lock.close()


if __name__ == "__main__":  # pragma: no cover
    raise SystemExit(main())
