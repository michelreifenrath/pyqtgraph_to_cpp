from __future__ import annotations

import json
import os
import stat
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]


def run_changed_examples(*args: str, env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    merged_env = os.environ.copy()
    merged_env.update(env or {})
    return subprocess.run(
        [sys.executable, str(REPO_ROOT / "scripts/run_changed_examples"), *args],
        cwd=REPO_ROOT,
        env=merged_env,
        text=True,
        capture_output=True,
        timeout=30,
    )


def make_executable(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8")
    path.chmod(path.stat().st_mode | stat.S_IXUSR)


def install_fake_tools(tmp_path: Path, *, runner_returncode: int = 0) -> tuple[Path, Path]:
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    calls_file = tmp_path / "calls.jsonl"
    recorder = (
        "import json, os, sys\n"
        "from pathlib import Path\n"
        f"calls_file = Path({json.dumps(str(calls_file))})\n"
        "entry = {\n"
        "    'tool': Path(sys.argv[0]).name,\n"
        "    'args': sys.argv[1:],\n"
        "    'cwd': Path.cwd().as_posix(),\n"
        "    'marker': os.environ.get('P1_09_ENV_MARKER'),\n"
        "    'qt': os.environ.get('QT_QPA_PLATFORM'),\n"
        "}\n"
        "calls_file.open('a', encoding='utf-8').write(json.dumps(entry) + '\\n')\n"
    )
    make_executable(
        bin_dir / "git",
        f"#!{sys.executable}\n"
        f"{recorder}"
        "if sys.argv[1:] != ['diff', '--name-only', os.environ.get('P1_09_EXPECTED_DIFF', 'origin/main...HEAD')]:\n"
        "    raise SystemExit(8)\n"
        "print(os.environ.get('P1_09_CHANGED_PATHS', ''), end='')\n",
    )
    make_executable(
        bin_dir / "ctest",
        f"#!{sys.executable}\n"
        f"{recorder}"
        f"raise SystemExit({runner_returncode})\n",
    )
    return bin_dir, calls_file


def read_calls(calls_file: Path) -> list[dict[str, object]]:
    if not calls_file.exists():
        return []
    return [json.loads(line) for line in calls_file.read_text(encoding="utf-8").splitlines()]


def test_P1_09_changed_simpleplot_runs_only_matching_ctest(tmp_path: Path) -> None:
    bin_dir, calls_file = install_fake_tools(tmp_path)

    result = run_changed_examples(
        env={
            "PATH": f"{bin_dir}{os.pathsep}{os.environ.get('PATH', '')}",
            "P1_09_CHANGED_PATHS": "examples/SimplePlot.cpp\ndocs/notes.md\n",
            "P1_09_ENV_MARKER": "propagated",
            "QT_QPA_PLATFORM": "caller-value",
        }
    )

    assert result.returncode == 0, result.stderr
    assert read_calls(calls_file) == [
        {
            "tool": "git",
            "args": ["diff", "--name-only", "origin/main...HEAD"],
            "cwd": REPO_ROOT.as_posix(),
            "marker": "propagated",
            "qt": "caller-value",
        },
        {
            "tool": "ctest",
            "args": [
                "--preset",
                "dev",
                "-R",
                "^pyqtgraph_cpp.examples.SimplePlot$",
                "--output-on-failure",
            ],
            "cwd": REPO_ROOT.as_posix(),
            "marker": "propagated",
            "qt": "offscreen",
        },
    ]


def test_P1_09_unrelated_changed_file_runs_no_example_command(tmp_path: Path) -> None:
    bin_dir, calls_file = install_fake_tools(tmp_path)

    result = run_changed_examples(
        env={
            "PATH": f"{bin_dir}{os.pathsep}{os.environ.get('PATH', '')}",
            "P1_09_CHANGED_PATHS": "README.md\nsrc/unrelated.cpp\n",
        }
    )

    assert result.returncode == 0, result.stderr
    assert [call["tool"] for call in read_calls(calls_file)] == ["git"]
    assert "No changed local examples" in result.stdout


def test_P1_09_unknown_explicit_selection_exits_2_without_git(tmp_path: Path) -> None:
    bin_dir, calls_file = install_fake_tools(tmp_path)

    result = run_changed_examples(
        "NotAnExample",
        env={"PATH": f"{bin_dir}{os.pathsep}{os.environ.get('PATH', '')}"},
    )

    assert result.returncode == 2
    assert "unknown example selection" in result.stderr
    assert read_calls(calls_file) == []


def test_P1_09_runner_failure_return_code_is_propagated(tmp_path: Path) -> None:
    bin_dir, calls_file = install_fake_tools(tmp_path, runner_returncode=17)

    result = run_changed_examples(
        "SimplePlot",
        env={
            "PATH": f"{bin_dir}{os.pathsep}{os.environ.get('PATH', '')}",
            "P1_09_ENV_MARKER": "propagated",
        },
    )

    assert result.returncode == 17
    assert read_calls(calls_file) == [
        {
            "tool": "ctest",
            "args": [
                "--preset",
                "dev",
                "-R",
                "^pyqtgraph_cpp.examples.SimplePlot$",
                "--output-on-failure",
            ],
            "cwd": REPO_ROOT.as_posix(),
            "marker": "propagated",
            "qt": "offscreen",
        }
    ]
