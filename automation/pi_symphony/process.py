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


def run(
    args: list[str] | str,
    *,
    cwd: str | Path | None = None,
    timeout: int = 300,
    check: bool = True,
    env: dict[str, str] | None = None,
    shell: bool = False,
) -> CommandResult:
    merged_env = os.environ.copy()
    if env:
        merged_env.update(env)
    completed = subprocess.run(
        args,
        cwd=str(cwd) if cwd is not None else None,
        timeout=timeout,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=merged_env,
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
