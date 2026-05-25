from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path
from typing import Any

import yaml

SCRIPT = Path("oracle/scripts/generate_P0_06_oracle_probe.py")
FIXTURE = Path("oracle/fixtures/P0_06/probe_contract.json")
MISMATCH = Path("oracle/fixtures/P0_06/mismatch_failure_example.txt")
REF = "pyqtgraph-0.14.0"
DOCS_URL = "https://pyqtgraph.readthedocs.io/"
CHECKOUT_PATH = "reference/pyqtgraph"
COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"


def run_cli(*args: str, root: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(SCRIPT.resolve()), *args],
        cwd=root,
        text=True,
        capture_output=True,
    )


def run_git(cwd: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["git", *args],
        cwd=cwd,
        text=True,
        capture_output=True,
        check=True,
    )


def write_reference_checkout(root: Path, version: str = "fixture-reference") -> str:
    checkout = root / CHECKOUT_PATH
    package = checkout / "pyqtgraph"
    package.mkdir(parents=True, exist_ok=True)
    (package / "__init__.py").write_text(
        f'__version__ = "{version}"\n', encoding="utf-8"
    )
    run_git(checkout, "init", "-q")
    run_git(checkout, "add", ".")
    run_git(
        checkout,
        "-c",
        "user.name=P0.06 Oracle",
        "-c",
        "user.email=oracle@example.invalid",
        "commit",
        "-q",
        "-m",
        "fake pyqtgraph reference",
    )
    run_git(checkout, "tag", REF)
    return run_git(checkout, "rev-parse", "HEAD").stdout.strip()


def write_source_lock(root: Path, *, commit: str | None = None) -> str:
    actual_commit = write_reference_checkout(root)
    pinned_commit = commit if commit is not None else actual_commit
    (root / "reference").mkdir(parents=True, exist_ok=True)
    (root / "reference" / "source.lock").write_text(
        yaml.safe_dump(
            {
                "repo": "fixture://pyqtgraph",
                "ref": REF,
                "pinned_commit": pinned_commit,
                "docs_url": DOCS_URL,
                "checkout_path": CHECKOUT_PATH,
            },
            sort_keys=False,
        ),
        encoding="utf-8",
    )
    return pinned_commit


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def test_P0_06_help_exposes_probe_cli_options() -> None:
    result = run_cli("--help")

    assert result.returncode == 0, result.stderr
    assert "--root" in result.stdout
    assert "--check" in result.stdout
    assert "--format" in result.stdout
    assert "--emit-mismatch-example" in result.stdout


def test_P0_06_generates_fixture_and_mismatch_example(tmp_path: Path) -> None:
    root = tmp_path / "workspace"
    commit = write_source_lock(root)

    result = run_cli("--root", str(root), "--format", "json", "--emit-mismatch-example")

    assert result.returncode == 0, result.stderr
    manifest = json.loads(result.stdout)
    assert manifest == {
        "fixture": FIXTURE.as_posix(),
        "mismatch_example": MISMATCH.as_posix(),
        "issue": "P0.06",
        "status": "written",
    }
    fixture = load_json(root / FIXTURE)
    assert fixture == {
        "schema_version": 1,
        "issue": "P0.06",
        "reference": {
            "repo": "fixture://pyqtgraph",
            "ref": REF,
            "pinned_commit": commit,
            "docs_url": DOCS_URL,
            "checkout_path": CHECKOUT_PATH,
            "pyqtgraph_version": "fixture-reference",
            "pyqtgraph_commit": commit,
        },
        "inputs": {
            "description": "Reusable pinned-PyQtGraph oracle probe template sanity values",
            "values": [1.25, -2.5, 3.75],
            "scale": 2.0,
            "offset": 0.5,
        },
        "expected": {
            "scaled_values": [3.0, -4.5, 8.0],
            "sum": 6.5,
            "count": 3,
        },
        "tolerance": {
            "absolute": 0.0,
            "relative": 0.0,
            "policy": "exact JSON numeric comparison for deterministic template outputs",
        },
    }
    example = (root / MISMATCH).read_text(encoding="utf-8")
    assert "oracle fixture mismatch" in example
    assert "oracle/fixtures/P0_06/probe_contract.json" in example
    assert "$.expected.scaled_values[0]" in example
    assert "expected fixture value" in example
    assert "actual probe value" in example
    assert "tolerance absolute=0.0 relative=0.0" in example


def test_P0_06_check_mode_reports_stale_fixture_with_json_path(tmp_path: Path) -> None:
    root = tmp_path / "workspace"
    write_source_lock(root)
    generated = run_cli("--root", str(root), "--emit-mismatch-example")
    assert generated.returncode == 0, generated.stderr
    fixture_path = root / FIXTURE
    fixture = load_json(fixture_path)
    fixture["expected"]["scaled_values"][0] = -999.0
    fixture_path.write_text(
        json.dumps(fixture, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    result = run_cli("--root", str(root), "--check")

    assert result.returncode == 1
    assert "oracle fixture mismatch" in result.stderr
    assert "oracle/fixtures/P0_06/probe_contract.json" in result.stderr
    assert "$.expected.scaled_values[0]" in result.stderr
    assert "expected fixture value: -999.0" in result.stderr
    assert "actual probe value: 3.0" in result.stderr
    assert "tolerance absolute=0.0 relative=0.0" in result.stderr


def test_P0_06_committed_fixture_is_current() -> None:
    result = run_cli("--check")

    assert result.returncode == 0, result.stderr
    assert "P0.06 oracle fixture is current" in result.stdout
