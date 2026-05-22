from __future__ import annotations

import json
import os
import shlex
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Any


@dataclass(frozen=True)
class CommandResult:
    args: list[str] | str
    returncode: int
    stdout: str
    stderr: str

    @property
    def combined_output(self) -> str:
        return (self.stdout + ("\n" if self.stdout and self.stderr else "") + self.stderr).strip()


def _parse_dotenv(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    if not path.exists():
        return values
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        if not key or key.startswith("#"):
            continue
        value = value.strip()
        if len(value) >= 2 and value[0] == value[-1] and value[0] in {'"', "'"}:
            value = value[1:-1]
        values[key] = value
    return values


def _subprocess_env(overrides: dict[str, str] | None = None) -> dict[str, str]:
    merged_env = os.environ.copy()
    hermes_home = Path(merged_env.get("HERMES_HOME") or Path.home() / ".hermes").expanduser()
    for key, value in _parse_dotenv(hermes_home / ".env").items():
        merged_env.setdefault(key, value)
    if "GH_TOKEN" not in merged_env and "GITHUB_TOKEN" in merged_env:
        merged_env["GH_TOKEN"] = merged_env["GITHUB_TOKEN"]
    if overrides:
        merged_env.update(overrides)
    return merged_env


def run(
    args: list[str] | str,
    *,
    cwd: str | Path | None = None,
    timeout: int = 300,
    check: bool = True,
    env: dict[str, str] | None = None,
    shell: bool = False,
) -> CommandResult:
    completed = subprocess.run(
        args,
        cwd=str(cwd) if cwd is not None else None,
        timeout=timeout,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=_subprocess_env(env),
        shell=shell,
    )
    result = CommandResult(args=args, returncode=completed.returncode, stdout=completed.stdout, stderr=completed.stderr)
    if check and completed.returncode != 0:
        rendered = args if isinstance(args, str) else " ".join(shlex.quote(str(a)) for a in args)
        raise RuntimeError(f"command failed ({completed.returncode}): {rendered}\n{result.combined_output}")
    return result


def run_json(args: list[str], *, cwd: str | Path | None = None, timeout: int = 300) -> Any:
    result = run(args, cwd=cwd, timeout=timeout, check=True)
    try:
        return json.loads(result.stdout or "null")
    except json.JSONDecodeError as exc:
        raise RuntimeError(f"command did not return JSON: {' '.join(args)}\n{result.stdout}\n{result.stderr}") from exc


def shell_join(args: list[str]) -> str:
    return " ".join(shlex.quote(str(arg)) for arg in args)
