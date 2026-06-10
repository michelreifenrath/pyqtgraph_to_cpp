"""P0.05 issue-owned file boundary checks."""

from __future__ import annotations

import importlib.machinery
import importlib.util
import json
import os
import stat
import subprocess
import sys
from pathlib import Path

import pytest
import yaml


REPO_ROOT = Path(__file__).resolve().parents[1]
CHECK_CHANGED_FILE_OWNERSHIP = REPO_ROOT / "scripts" / "check_changed_file_ownership"
GATE = REPO_ROOT / "scripts" / "gate"


def load_ownership_module():
    loader = importlib.machinery.SourceFileLoader(
        "check_changed_file_ownership",
        str(CHECK_CHANGED_FILE_OWNERSHIP),
    )
    spec = importlib.util.spec_from_loader(loader.name, loader)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def load_gate_module():
    loader = importlib.machinery.SourceFileLoader("gate", str(GATE))
    spec = importlib.util.spec_from_loader(loader.name, loader)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def write_yaml(path: Path, data: dict) -> None:
    path.write_text(yaml.safe_dump(data, sort_keys=False), encoding="utf-8")


def write_workflow(path: Path) -> None:
    path.write_text(
        "\n".join(
            [
                "---",
                "policy:",
                "  shared_integration_files:",
                "    - CMakeLists.txt",
                "    - reports/agents/<issue-code>.md",
                "validation:",
                "  commands: []",
                "autoreview:",
                "  base: origin/main",
                "---",
                "# test",
                "",
            ]
        ),
        encoding="utf-8",
    )


def run_checker(
    root: Path,
    *,
    registry: Path,
    workflow: Path,
    base: str = "origin/main",
    branch: str = "ai/issue-99",
    changed_files: list[str] | None = None,
) -> subprocess.CompletedProcess[str]:
    args = [
        sys.executable,
        str(CHECK_CHANGED_FILE_OWNERSHIP),
        "--registry",
        str(registry),
        "--workflow",
        str(workflow),
        "--base",
        base,
        "--branch",
        branch,
    ]
    for changed_file in changed_files or []:
        args.extend(["--changed-file", changed_file])
    return subprocess.run(args, cwd=root, text=True, capture_output=True)


def make_executable(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8")
    path.chmod(path.stat().st_mode | stat.S_IXUSR)


def test_P0_05_changed_file_ownership_passes_for_in_scope_paths(tmp_path: Path) -> None:
    registry = tmp_path / "ownership.yaml"
    workflow = tmp_path / "WORKFLOW.md"
    write_workflow(workflow)
    write_yaml(
        registry,
        {
            "version": 1,
            "claims": [
                {
                    "issue": "P0.05",
                    "branch": "ai/issue-99",
                    "status": "active",
                    "owned_files": [
                        "ownership.yaml",
                        "scripts/**ownership*",
                        "tests/test_board_policy.py",
                    ],
                }
            ],
        },
    )

    result = run_checker(
        tmp_path,
        registry=registry,
        workflow=workflow,
        changed_files=[
            "ownership.yaml",
            "scripts/check_changed_file_ownership",
            "tests/test_board_policy.py",
        ],
    )

    assert result.returncode == 0, result.stderr
    assert "changed-file ownership check passed" in result.stdout


def test_P0_05_changed_file_ownership_fails_for_out_of_scope_path(tmp_path: Path) -> None:
    registry = tmp_path / "ownership.yaml"
    workflow = tmp_path / "WORKFLOW.md"
    write_workflow(workflow)
    write_yaml(
        registry,
        {
            "version": 1,
            "claims": [
                {
                    "issue": "P0.05",
                    "branch": "ai/issue-99",
                    "status": "active",
                    "owned_files": ["scripts/**ownership*"],
                }
            ],
        },
    )

    result = run_checker(
        tmp_path,
        registry=registry,
        workflow=workflow,
        changed_files=["src/cppqtgraph/Foo.cpp"],
    )

    assert result.returncode != 0
    assert "out-of-scope changed file: src/cppqtgraph/Foo.cpp" in result.stderr


def test_P0_05_changed_file_ownership_rejects_missing_active_claim(tmp_path: Path) -> None:
    registry = tmp_path / "ownership.yaml"
    workflow = tmp_path / "WORKFLOW.md"
    write_workflow(workflow)
    write_yaml(registry, {"version": 1, "claims": []})

    result = run_checker(
        tmp_path,
        registry=registry,
        workflow=workflow,
        changed_files=["ownership.yaml"],
    )

    assert result.returncode != 0
    assert "missing active ownership claim for branch ai/issue-99" in result.stderr


def test_P0_05_changed_file_ownership_rejects_stale_inactive_claim(tmp_path: Path) -> None:
    registry = tmp_path / "ownership.yaml"
    workflow = tmp_path / "WORKFLOW.md"
    write_workflow(workflow)
    write_yaml(
        registry,
        {
            "version": 1,
            "claims": [
                {
                    "issue": "P0.05",
                    "branch": "ai/issue-99",
                    "status": "done",
                    "owned_files": ["ownership.yaml"],
                }
            ],
        },
    )

    result = run_checker(
        tmp_path,
        registry=registry,
        workflow=workflow,
        changed_files=["ownership.yaml"],
    )

    assert result.returncode != 0
    assert "missing active ownership claim for branch ai/issue-99" in result.stderr


def test_P0_05_changed_file_ownership_allows_shared_integration_paths(tmp_path: Path) -> None:
    ownership = load_ownership_module()
    patterns = ownership.collect_allowed_patterns(
        {
            "issue": "P0.05",
            "branch": "ai/issue-99",
            "status": "active",
            "owned_files": ["scripts/**ownership*"],
        },
        workflow_path=write_workflow_and_return(tmp_path),
    )

    assert ownership.path_is_allowed("CMakeLists.txt", patterns)
    assert ownership.path_is_allowed("reports/agents/P0.05.md", patterns)


def write_workflow_and_return(root: Path) -> Path:
    workflow = root / "WORKFLOW.md"
    write_workflow(workflow)
    return workflow


def test_P0_05_changed_file_ownership_rejects_inconsistent_duplicate_claims(tmp_path: Path) -> None:
    ownership = load_ownership_module()
    claims = [
        {
            "issue": "P0.05",
            "branch": "ai/issue-99",
            "status": "active",
            "owned_files": ["ownership.yaml"],
        },
        {
            "issue": "P0.06",
            "branch": "ai/issue-99",
            "status": "active",
            "owned_files": ["scripts/**ownership*"],
        },
    ]

    with pytest.raises(ownership.OwnershipBoundaryError, match="multiple active claims"):
        ownership.find_active_claim(claims, "ai/issue-99")


def test_P0_05_gate_commit_rejects_out_of_scope_branch_changes(tmp_path: Path) -> None:
    workflow = tmp_path / "WORKFLOW.md"
    registry = tmp_path / "ownership.yaml"
    write_workflow(workflow)
    write_yaml(
        registry,
        {
            "version": 1,
            "claims": [
                {
                    "issue": "P0.05",
                    "branch": "ai/issue-99",
                    "status": "active",
                    "owned_files": ["ownership.yaml"],
                }
            ],
        },
    )

    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    make_executable(
        bin_dir / "git",
        f"#!{sys.executable}\n"
        "import sys\n"
        "args = sys.argv[1:]\n"
        "if args == ['rev-parse', '--abbrev-ref', 'HEAD']:\n"
        "    print('ai/issue-99')\n"
        "    raise SystemExit(0)\n"
        "if args == ['diff', '--name-only', 'origin/main...HEAD']:\n"
        "    print('src/cppqtgraph/Foo.cpp')\n"
        "    raise SystemExit(0)\n"
        "if args in (\n"
        "    ['diff', '--check'],\n"
        "    ['diff', '--cached', '--check'],\n"
        "    ['diff', '--check', 'origin/main...HEAD'],\n"
        "):\n"
        "    raise SystemExit(0)\n"
        "raise SystemExit(9)\n",
    )

    env = os.environ.copy()
    env["PATH"] = f"{bin_dir}{os.pathsep}{env.get('PATH', '')}"
    env["PGBOOT_OWNERSHIP_REGISTRY"] = str(registry)
    env["PGBOOT_FORCE_OWNERSHIP_CHECK"] = "1"

    result = subprocess.run(
        [sys.executable, str(GATE), "commit", "--workflow", str(workflow)],
        cwd=tmp_path,
        env=env,
        text=True,
        capture_output=True,
        timeout=30,
    )

    assert result.returncode != 0
    assert "changed-file ownership check" in result.stdout
    assert "out-of-scope changed file: src/cppqtgraph/Foo.cpp" in result.stderr
