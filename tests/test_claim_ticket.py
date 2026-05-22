from pathlib import Path
import subprocess
import sys

import yaml


REPO_ROOT = Path(__file__).resolve().parents[1]
CLAIM_TICKET = REPO_ROOT / "scripts" / "claim_ticket"


def run_claim_ticket(*args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(CLAIM_TICKET), *args],
        cwd=REPO_ROOT,
        text=True,
        capture_output=True,
    )


def write_registry(path: Path, data: dict) -> None:
    path.write_text(yaml.safe_dump(data, sort_keys=False), encoding="utf-8")


def read_registry(path: Path) -> dict:
    return yaml.safe_load(path.read_text(encoding="utf-8"))


def test_rejects_active_owned_file_overlap(tmp_path: Path):
    registry = tmp_path / "ownership.yaml"
    original = {
        "version": 1,
        "claims": [
            {
                "issue": "PGBOOT-001",
                "branch": "ai/issue-1",
                "status": "active",
                "owned_files": ["src/existing.cpp"],
            }
        ],
    }
    write_registry(registry, original)

    result = run_claim_ticket(
        "--registry",
        str(registry),
        "--issue",
        "PGBOOT-004",
        "--branch",
        "ai/issue-4",
        "--file",
        "src/existing.cpp",
    )

    assert result.returncode != 0
    assert "overlaps active claim" in result.stderr
    assert "PGBOOT-001" in result.stderr
    assert "src/existing.cpp" in result.stderr
    assert read_registry(registry) == original


def test_allows_non_overlapping_claim_and_records_it(tmp_path: Path):
    registry = tmp_path / "ownership.yaml"
    original_claim = {
        "issue": "PGBOOT-001",
        "branch": "ai/issue-1",
        "status": "active",
        "owned_files": ["src/existing.cpp"],
    }
    write_registry(registry, {"version": 1, "claims": [original_claim]})

    result = run_claim_ticket(
        "--registry",
        str(registry),
        "--issue",
        "PGBOOT-004",
        "--branch",
        "ai/issue-4",
        "--file",
        "tests/new_test.cpp",
        "--file",
        "src/new.cpp",
    )

    assert result.returncode == 0, result.stderr
    assert "claim recorded" in result.stdout
    assert read_registry(registry) == {
        "version": 1,
        "claims": [
            original_claim,
            {
                "issue": "PGBOOT-004",
                "branch": "ai/issue-4",
                "status": "active",
                "owned_files": ["tests/new_test.cpp", "src/new.cpp"],
            },
        ],
    }


def test_ignores_non_active_claim_overlaps(tmp_path: Path):
    registry = tmp_path / "ownership.yaml"
    inactive_claim = {
        "issue": "PGBOOT-001",
        "branch": "ai/issue-1",
        "status": "done",
        "owned_files": ["src/reusable.cpp"],
    }
    write_registry(registry, {"version": 1, "claims": [inactive_claim]})

    result = run_claim_ticket(
        "--registry",
        str(registry),
        "--issue",
        "PGBOOT-005",
        "--branch",
        "ai/issue-5",
        "--file",
        "src/reusable.cpp",
    )

    assert result.returncode == 0, result.stderr
    assert read_registry(registry)["claims"] == [
        inactive_claim,
        {
            "issue": "PGBOOT-005",
            "branch": "ai/issue-5",
            "status": "active",
            "owned_files": ["src/reusable.cpp"],
        },
    ]


def test_creates_registry_when_missing(tmp_path: Path):
    registry = tmp_path / "missing" / "ownership.yaml"

    result = run_claim_ticket(
        "--registry",
        str(registry),
        "--issue",
        "PGBOOT-006",
        "--branch",
        "ai/issue-6",
        "--file",
        "src/new.cpp",
    )

    assert result.returncode == 0, result.stderr
    assert read_registry(registry) == {
        "version": 1,
        "claims": [
            {
                "issue": "PGBOOT-006",
                "branch": "ai/issue-6",
                "status": "active",
                "owned_files": ["src/new.cpp"],
            }
        ],
    }
