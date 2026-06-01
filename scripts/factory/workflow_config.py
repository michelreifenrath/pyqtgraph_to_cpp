from __future__ import annotations

import os
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from types import SimpleNamespace
from typing import Any

try:
    import yaml
except Exception:  # pragma: no cover - import failure is reported through ConfigError at runtime
    yaml = None


class ConfigError(ValueError):
    pass


@dataclass(frozen=True)
class CommandResult:
    args: list[str] | str
    returncode: int
    stdout: str
    stderr: str

    @property
    def combined_output(self) -> str:
        return (self.stdout + ("\n" if self.stdout and self.stderr else "") + self.stderr).strip()


@dataclass(frozen=True)
class LoadedWorkflow:
    config: Any
    path: Path
    repo_root: Path


def split_front_matter(text: str) -> tuple[str, str]:
    if not text.startswith("---\n"):
        raise ConfigError("WORKFLOW.md must start with YAML front matter")
    try:
        _, front_matter, body = text.split("---", 2)
    except ValueError as exc:
        raise ConfigError("WORKFLOW.md front matter is not closed") from exc
    return front_matter.strip(), body.lstrip("\n")


def _namespace(value: Any) -> Any:
    if isinstance(value, dict):
        return SimpleNamespace(**{str(key): _namespace(item) for key, item in value.items()})
    if isinstance(value, list):
        return [_namespace(item) for item in value]
    return value


def _section(data: dict[str, Any], name: str) -> dict[str, Any]:
    value = data.get(name, {})
    if value is None:
        value = {}
    if not isinstance(value, dict):
        raise ConfigError(f"{name} must be a mapping")
    return value


def parse_workflow_text(text: str) -> Any:
    if yaml is None:
        raise ConfigError("PyYAML is required to parse WORKFLOW.md")
    front_matter, _body = split_front_matter(text)
    loaded = yaml.safe_load(front_matter) or {}
    if not isinstance(loaded, dict):
        raise ConfigError("WORKFLOW.md front matter must be a mapping")

    validation = _section(loaded, "validation")
    commands = validation.get("commands", [])
    if commands is None:
        commands = []
    if not isinstance(commands, list) or not all(isinstance(command, str) for command in commands):
        raise ConfigError("validation.commands must be a list of shell command strings")
    validation.setdefault("commands", commands)
    validation.setdefault("diff_check", True)

    autoreview = _section(loaded, "autoreview")
    autoreview.setdefault("command", "autoreview")
    autoreview.setdefault("mode", "branch")
    autoreview.setdefault("base", "origin/main")
    autoreview.setdefault("require_clean", False)

    policy = _section(loaded, "policy")
    policy.setdefault("auto_merge", False)

    return _namespace(loaded)


def load_workflow(path: str | Path = "WORKFLOW.md") -> Any:
    return parse_workflow_text(Path(path).read_text(encoding="utf-8"))


def load_workflow_with_context(path: str | Path = "WORKFLOW.md") -> LoadedWorkflow:
    workflow_path = Path(path).resolve()
    return LoadedWorkflow(
        config=parse_workflow_text(workflow_path.read_text(encoding="utf-8")),
        path=workflow_path,
        repo_root=workflow_path.parent,
    )


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


def _is_gh_command(args: list[str] | str) -> bool:
    return not isinstance(args, str) and bool(args) and str(args[0]) == "gh"


def _is_requires_auth_401(output: str) -> bool:
    normalized = output.lower()
    return "requires authentication" in normalized or "http 401" in normalized or '"status":"401"' in normalized


def _gh_auth_probe(env: dict[str, str]) -> bool:
    probe = subprocess.run(
        ["gh", "api", "user", "--jq", ".login"],
        timeout=30,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
    )
    return probe.returncode == 0 and bool((probe.stdout or "").strip())


def run(
    args: list[str] | str,
    *,
    cwd: str | Path | None = None,
    timeout: int = 300,
    check: bool = True,
    env: dict[str, str] | None = None,
    shell: bool = False,
) -> CommandResult:
    merged_env = _subprocess_env(env)
    attempts = 0
    while True:
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
        if (
            check
            and completed.returncode != 0
            and _is_gh_command(args)
            and _is_requires_auth_401(result.combined_output)
            and attempts < 3
            and _gh_auth_probe(merged_env)
        ):
            attempts += 1
            time.sleep(min(30, 2**attempts))
            continue
        if check and completed.returncode != 0:
            raise RuntimeError(f"command failed ({completed.returncode}): {args}\n{result.combined_output}")
        return result
