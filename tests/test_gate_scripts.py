"""Tests for PGBOOT-004 gate and autoreview scripts."""

from __future__ import annotations

import json
import os
import stat
import subprocess
import sys
import time
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]


def write_workflow(
    path: Path,
    *,
    commands: list[str] | None = None,
    autoreview_command: str = "autoreview",
    require_clean: bool = False,
) -> None:
    commands = commands or [f"{sys.executable} -c 'print(\"validated\")'"]
    command_lines = "\n".join(f"    - {json.dumps(command)}" for command in commands)
    path.write_text(
        "\n".join(
            [
                "---",
                "tracker:",
                "  kind: github",
                "  repo: example/repo",
                "workspace:",
                f"  root: {path.parent.as_posix()}",
                "  strategy: git-worktree",
                "  base_branch: main",
                "pi:",
                "  command: pi",
                "  use_subagents: true",
                "autoreview:",
                "  enabled: true",
                f"  command: {json.dumps(autoreview_command)}",
                "  engine: codex",
                "  mode: commit",
                "  base: origin/main",
                f"  require_clean: {str(require_clean).lower()}",
                "  advisory: true",
                "  mandatory_gate: true",
                "policy:",
                "  never_push_to_main: true",
                "  auto_merge: false",
                "validation:",
                "  diff_check: true",
                "  commands:",
                command_lines,
                "kanban:",
                "  board_slug: pyqtgraph-to-cpp",
                "  board_scope: project",
                "  tenant_strategy: tags",
                "  default_tenant: core",
                "  tenant_label_prefix: 'tenant:'",
                "  tag_label_prefix: 'tag:'",
                "---",
                "# Test workflow",
                "",
            ]
        ),
        encoding="utf-8",
    )


def run_script(
    script: str, *args: str, env: dict[str, str] | None = None
) -> subprocess.CompletedProcess[str]:
    merged_env = os.environ.copy()
    merged_env.update(env or {})
    return subprocess.run(
        [sys.executable, str(REPO_ROOT / script), *args],
        cwd=REPO_ROOT,
        env=merged_env,
        text=True,
        capture_output=True,
        timeout=30,
    )


def make_executable(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8")
    path.chmod(path.stat().st_mode | stat.S_IXUSR)


def test_gate_help_lists_required_modes() -> None:
    result = run_script("scripts/gate", "--help")

    assert result.returncode == 0
    for mode in ["focus", "commit", "merge", "visual"]:
        assert mode in result.stdout
    assert "performance" not in result.stdout


def test_workflow_pre_pr_autoreview_uses_branch_mode() -> None:
    workflow_text = (REPO_ROOT / "WORKFLOW.md").read_text(encoding="utf-8")

    assert "scripts/run_autoreview --mode branch" in workflow_text
    assert "scripts/run_autoreview --mode commit" not in workflow_text


def test_gate_focus_runs_configured_validation_and_writes_summary(
    tmp_path: Path,
) -> None:
    workflow = tmp_path / "WORKFLOW.md"
    marker = tmp_path / "validated.txt"
    reports = tmp_path / "reports"
    code = f"from pathlib import Path; Path({json.dumps(str(marker))}).write_text('ok')"
    write_workflow(workflow, commands=[f"{sys.executable} -c {json.dumps(code)}"])

    result = run_script(
        "scripts/gate",
        "focus",
        "--workflow",
        str(workflow),
        "--reports-dir",
        str(reports),
    )

    assert result.returncode == 0, result.stderr
    assert marker.read_text(encoding="utf-8") == "ok"
    summary = json.loads((reports / "focus-summary.json").read_text(encoding="utf-8"))
    assert summary["mode"] == "focus"
    assert summary["status"] == "passed"
    assert summary["commands"][0]["returncode"] == 0


def test_gate_commit_runs_diff_check_before_validation(tmp_path: Path) -> None:
    workflow = tmp_path / "WORKFLOW.md"
    reports = tmp_path / "reports"
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    order_file = tmp_path / "order.txt"
    make_executable(
        bin_dir / "git",
        f"#!{sys.executable}\n"
        "import sys\n"
        "from pathlib import Path\n"
        f"order_file = Path({json.dumps(str(order_file))})\n"
        "args = sys.argv[1:]\n"
        "order_file.open('a', encoding='utf-8').write(' '.join(args) + '\\n')\n"
        "if args not in (\n"
        "    ['diff', '--check'],\n"
        "    ['diff', '--cached', '--check'],\n"
        "    ['diff', '--check', 'origin/main...HEAD'],\n"
        "):\n"
        "    raise SystemExit(9)\n",
    )
    code = f"from pathlib import Path; Path({json.dumps(str(order_file))}).open('a').write('validation\\n')"
    write_workflow(workflow, commands=[f"{sys.executable} -c {json.dumps(code)}"])

    result = run_script(
        "scripts/gate",
        "commit",
        "--workflow",
        str(workflow),
        "--reports-dir",
        str(reports),
        env={"PATH": f"{bin_dir}{os.pathsep}{os.environ.get('PATH', '')}"},
    )

    assert result.returncode == 0, result.stderr
    assert order_file.read_text(encoding="utf-8").splitlines() == [
        "diff --check",
        "diff --cached --check",
        "diff --check origin/main...HEAD",
        "validation",
    ]


def test_gate_defaults_reports_to_ignored_hermes_logs(tmp_path: Path) -> None:
    workflow = tmp_path / "WORKFLOW.md"
    write_workflow(workflow)

    result = run_script("scripts/gate", "focus", "--workflow", str(workflow))

    assert result.returncode == 0, result.stderr
    reports = tmp_path / ".hermes" / "pi-symphony" / "logs" / "gates"
    assert (reports / "focus-summary.json").exists()
    assert not (tmp_path / "reports" / "gates" / "focus-summary.json").exists()


def test_gate_commit_stops_on_first_failure(tmp_path: Path) -> None:
    workflow = tmp_path / "WORKFLOW.md"
    reports = tmp_path / "reports"
    first_code = "import sys; sys.exit(7)"
    second_code = 'print("must not run")'
    first_command = f"{sys.executable} -c {json.dumps(first_code)}"
    second_command = f"{sys.executable} -c {json.dumps(second_code)}"
    write_workflow(workflow, commands=[first_command, second_command])

    result = run_script(
        "scripts/gate",
        "focus",
        "--workflow",
        str(workflow),
        "--reports-dir",
        str(reports),
    )

    assert result.returncode == 7
    assert "failed" in result.stderr.lower()
    summary = json.loads((reports / "focus-summary.json").read_text(encoding="utf-8"))
    assert summary["status"] == "failed"
    assert len(summary["commands"]) == 1


def test_gate_times_out_safely(tmp_path: Path) -> None:
    workflow = tmp_path / "WORKFLOW.md"
    reports = tmp_path / "reports"
    slow_code = "import time; time.sleep(2)"
    write_workflow(workflow, commands=[f"{sys.executable} -c {json.dumps(slow_code)}"])

    result = run_script(
        "scripts/gate",
        "focus",
        "--workflow",
        str(workflow),
        "--reports-dir",
        str(reports),
        "--timeout",
        "1",
    )

    assert result.returncode == 124
    assert "timed out" in result.stderr.lower()
    assert "traceback" not in result.stderr.lower()
    summary = json.loads((reports / "focus-summary.json").read_text(encoding="utf-8"))
    assert summary["status"] == "failed"
    assert summary["timeout_seconds"] == 1
    assert len(summary["commands"]) == 1
    assert summary["commands"][0]["returncode"] == 124
    log = (reports / "focus-1.log").read_text(encoding="utf-8")
    assert "timed out" in log.lower()
    assert "traceback" not in log.lower()


def test_gate_timeout_kills_shell_process_group(tmp_path: Path) -> None:
    workflow = tmp_path / "WORKFLOW.md"
    reports = tmp_path / "reports"
    marker = tmp_path / "leaked-child.txt"
    child_code = "; ".join(
        [
            "import time",
            "from pathlib import Path",
            "time.sleep(2)",
            f"Path({json.dumps(str(marker))}).write_text('leaked', encoding='utf-8')",
        ]
    )
    write_workflow(
        workflow, commands=[f"{sys.executable} -c {json.dumps(child_code)} & wait"]
    )

    result = run_script(
        "scripts/gate",
        "focus",
        "--workflow",
        str(workflow),
        "--reports-dir",
        str(reports),
        "--timeout",
        "1",
    )
    time.sleep(2.25)

    assert result.returncode == 124
    assert not marker.exists()


def test_gate_dry_run_command_plans(tmp_path: Path) -> None:
    workflow = tmp_path / "WORKFLOW.md"
    reports = tmp_path / "reports"
    validation = f"{sys.executable} -c 'print(\"validated\")'"
    write_workflow(workflow, commands=[validation])

    expected = {
        "focus": [validation],
        "commit": [
            "git diff --check",
            "git diff --cached --check",
            "git diff --check origin/main...HEAD",
            validation,
        ],
        "merge": [
            "git diff --check",
            "git diff --cached --check",
            "git diff --check origin/main...HEAD",
            validation,
            "cmake --preset dev",
            "cmake --build --preset dev --parallel",
            "ctest --preset dev --output-on-failure",
        ],
    }

    for mode, commands in expected.items():
        result = run_script(
            "scripts/gate",
            mode,
            "--workflow",
            str(workflow),
            "--reports-dir",
            str(reports),
            "--dry-run",
        )
        assert result.returncode == 0, result.stderr
        assert result.stdout.splitlines() == commands
    assert not reports.exists()


def test_gate_visual_dry_run_targets_example_pytest(tmp_path: Path) -> None:
    workflow = tmp_path / "WORKFLOW.md"
    reports = tmp_path / "reports"
    write_workflow(workflow)

    result = run_script(
        "scripts/gate",
        "visual",
        "SimplePlot",
        "--workflow",
        str(workflow),
        "--reports-dir",
        str(reports),
        "--dry-run",
    )

    assert result.returncode == 0, result.stderr
    assert result.stdout.splitlines() == [
        "python3 -m pytest tests/examples/test_SimplePlot_visual.py -q"
    ]
    assert not reports.exists()


def test_gate_visual_requires_example_name() -> None:
    result = run_script("scripts/gate", "visual", "--dry-run")

    assert result.returncode == 2
    assert "requires an example name" in result.stderr


def test_gate_visual_requires_known_target(tmp_path: Path) -> None:
    workflow = tmp_path / "WORKFLOW.md"
    write_workflow(workflow)

    result = run_script(
        "scripts/gate",
        "visual",
        "MissingCase",
        "--workflow",
        str(workflow),
        "--dry-run",
    )

    assert result.returncode == 2
    assert "unsupported visual gate target" in result.stderr


def test_run_autoreview_fails_safely_when_tools_unavailable(tmp_path: Path) -> None:
    workflow = tmp_path / "WORKFLOW.md"
    write_workflow(workflow, autoreview_command="definitely-not-autoreview")

    result = run_script(
        "scripts/run_autoreview",
        "--workflow",
        str(workflow),
        "--reports-dir",
        str(tmp_path / "reports"),
        env={"PATH": str(tmp_path / "empty-bin")},
    )

    assert result.returncode == 127
    assert "neither autoreview nor codex review is available" in result.stderr.lower()
    assert "traceback" not in result.stderr.lower()
    summary = json.loads(
        (tmp_path / "reports" / "autoreview-summary.json").read_text(encoding="utf-8")
    )
    assert summary["status"] == "unavailable"


def test_run_autoreview_defaults_reports_to_ignored_hermes_logs(tmp_path: Path) -> None:
    workflow = tmp_path / "WORKFLOW.md"
    write_workflow(workflow, autoreview_command="definitely-not-autoreview")

    result = run_script(
        "scripts/run_autoreview",
        "--workflow",
        str(workflow),
        env={"PATH": str(tmp_path / "empty-bin")},
    )

    assert result.returncode == 127
    reports = tmp_path / ".hermes" / "pi-symphony" / "logs" / "gates"
    assert (reports / "autoreview-summary.json").exists()
    assert not (tmp_path / "reports" / "gates" / "autoreview-summary.json").exists()


def test_run_autoreview_requires_clean_worktree_before_review(
    tmp_path: Path,
) -> None:
    workflow = tmp_path / "WORKFLOW.md"
    reports = tmp_path / "reports"
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    received = tmp_path / "autoreview-called.txt"
    make_executable(
        bin_dir / "git",
        f"#!{sys.executable}\n"
        "import sys\n"
        "if sys.argv[1:] == ['status', '--porcelain']:\n"
        "    print('A  staged.txt')\n"
        "    print(' M dirty.txt')\n"
        "    print('?? new.txt')\n"
        "    raise SystemExit(0)\n"
        "raise SystemExit(99)\n",
    )
    make_executable(
        bin_dir / "autoreview",
        f"#!{sys.executable}\n"
        "from pathlib import Path\n"
        f"Path({json.dumps(str(received))}).write_text('called')\n",
    )
    write_workflow(workflow, autoreview_command="autoreview", require_clean=True)

    result = run_script(
        "scripts/run_autoreview",
        "--workflow",
        str(workflow),
        "--reports-dir",
        str(reports),
        env={"PATH": f"{bin_dir}{os.pathsep}{os.environ.get('PATH', '')}"},
    )

    assert result.returncode != 0
    assert "clean worktree" in result.stderr.lower()
    assert not received.exists()
    assert not (reports / "autoreview-prompt.md").exists()
    summary = json.loads(
        (reports / "autoreview-summary.json").read_text(encoding="utf-8")
    )
    assert summary["status"] == "dirty_worktree"


def test_run_autoreview_times_out_safely(tmp_path: Path) -> None:
    workflow = tmp_path / "WORKFLOW.md"
    reports = tmp_path / "reports"
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    make_executable(
        bin_dir / "autoreview",
        f"#!{sys.executable}\nimport time\ntime.sleep(2)\n",
    )
    write_workflow(workflow, autoreview_command="autoreview")

    result = run_script(
        "scripts/run_autoreview",
        "--workflow",
        str(workflow),
        "--reports-dir",
        str(reports),
        "--timeout",
        "1",
        env={"PATH": f"{bin_dir}{os.pathsep}{os.environ.get('PATH', '')}"},
    )

    assert result.returncode == 124
    assert "timed out" in result.stderr.lower()
    assert "traceback" not in result.stderr.lower()
    summary = json.loads(
        (reports / "autoreview-summary.json").read_text(encoding="utf-8")
    )
    assert summary["status"] == "timed_out"
    assert summary["timeout_seconds"] == 1


def test_run_autoreview_timeout_kills_reviewer_process_group(tmp_path: Path) -> None:
    workflow = tmp_path / "WORKFLOW.md"
    reports = tmp_path / "reports"
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    marker = tmp_path / "leaked-review-child.txt"
    child_script = tmp_path / "leaky_review_child.py"
    child_script.write_text(
        "import time\n"
        "from pathlib import Path\n"
        "time.sleep(2)\n"
        f"Path({json.dumps(str(marker))}).write_text('leaked', encoding='utf-8')\n",
        encoding="utf-8",
    )
    make_executable(
        bin_dir / "autoreview",
        f"#!{sys.executable}\n"
        "import subprocess\n"
        "import sys\n"
        "import time\n"
        f"subprocess.Popen([sys.executable, {json.dumps(str(child_script))}])\n"
        "time.sleep(2)\n",
    )
    write_workflow(workflow, autoreview_command="autoreview")

    result = run_script(
        "scripts/run_autoreview",
        "--workflow",
        str(workflow),
        "--reports-dir",
        str(reports),
        "--timeout",
        "1",
        env={"PATH": f"{bin_dir}{os.pathsep}{os.environ.get('PATH', '')}"},
    )
    time.sleep(2.25)

    assert result.returncode == 124
    assert not marker.exists()


def test_run_autoreview_maps_merge_mode_to_branch_for_autoreview(
    tmp_path: Path,
) -> None:
    workflow = tmp_path / "WORKFLOW.md"
    reports = tmp_path / "reports"
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    received = tmp_path / "received.txt"
    make_executable(
        bin_dir / "autoreview",
        f"#!{sys.executable}\n"
        "import sys\n"
        "from pathlib import Path\n"
        f"Path({json.dumps(str(received))}).write_text('\\n'.join(sys.argv[1:]) + '\\n', encoding='utf-8')\n",
    )
    write_workflow(workflow, autoreview_command="autoreview")

    result = run_script(
        "scripts/run_autoreview",
        "--workflow",
        str(workflow),
        "--reports-dir",
        str(reports),
        "--mode",
        "merge",
        "--base",
        "origin/main",
        env={"PATH": f"{bin_dir}{os.pathsep}{os.environ.get('PATH', '')}"},
    )

    assert result.returncode == 0, result.stderr
    args = received.read_text(encoding="utf-8").splitlines()
    assert args[:4] == ["--mode", "branch", "--base", "origin/main"]
    summary = json.loads(
        (reports / "autoreview-summary.json").read_text(encoding="utf-8")
    )
    assert summary["mode"] == "merge"


def test_run_autoreview_uses_available_autoreview_and_writes_outputs(
    tmp_path: Path,
) -> None:
    workflow = tmp_path / "WORKFLOW.md"
    reports = tmp_path / "reports"
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    received = tmp_path / "received.txt"
    make_executable(
        bin_dir / "autoreview",
        f"#!/bin/sh\nprintf '%s\\n' \"$@\" > {received}\nexit 0\n",
    )
    write_workflow(workflow, autoreview_command="autoreview")

    result = run_script(
        "scripts/run_autoreview",
        "--workflow",
        str(workflow),
        "--reports-dir",
        str(reports),
        env={"PATH": f"{bin_dir}{os.pathsep}{os.environ.get('PATH', '')}"},
    )

    assert result.returncode == 0, result.stderr
    args = received.read_text(encoding="utf-8").splitlines()
    assert args[:4] == ["--mode", "commit", "--base", "origin/main"]
    assert "--prompt-file" in args
    assert "--json-output" in args
    assert (reports / "autoreview-prompt.md").exists()
    summary = json.loads(
        (reports / "autoreview-summary.json").read_text(encoding="utf-8")
    )
    assert summary["status"] == "passed"
