"""Tests for the scripts/validate_local baseline runner."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]


def run_validate(*args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(REPO_ROOT / "scripts/validate_local"), *args],
        cwd=REPO_ROOT,
        text=True,
        capture_output=True,
        timeout=30,
    )


def test_list_steps_matches_agents_baseline_order() -> None:
    result = run_validate("--list-steps")

    assert result.returncode == 0
    lines = [line for line in result.stdout.splitlines() if line.strip()]
    names = [line.split(":", 1)[0] for line in lines]
    assert names == ["pytest", "configure", "build", "ctest", "git-diff-check"]

    commands = dict(line.split(": ", 1) for line in lines)
    assert commands["pytest"].endswith("-m pytest -q")
    assert commands["configure"] == "cmake --preset dev"
    assert commands["build"] == "cmake --build --preset dev --parallel"
    assert commands["ctest"] == "ctest --preset dev --output-on-failure"
    assert commands["git-diff-check"] == "git diff --check"


def test_list_steps_applies_requested_preset() -> None:
    result = run_validate("--list-steps", "--preset", "ci-linux")

    assert result.returncode == 0
    assert "cmake --preset ci-linux" in result.stdout
    assert "cmake --build --preset ci-linux --parallel" in result.stdout
    assert "ctest --preset ci-linux --output-on-failure" in result.stdout


def test_script_is_executable() -> None:
    script = REPO_ROOT / "scripts/validate_local"
    assert script.stat().st_mode & 0o111, "scripts/validate_local must be executable"
