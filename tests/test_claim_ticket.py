from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import subprocess
import sys

import pytest
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


@pytest.mark.parametrize(
    ("option", "blank_value"),
    [("--issue", ""), ("--branch", "   ")],
)
def test_rejects_blank_issue_or_branch_without_modifying_registry(
    tmp_path: Path, option: str, blank_value: str
):
    registry = tmp_path / "ownership.yaml"
    original = {"version": 1, "claims": []}
    write_registry(registry, original)
    args = [
        "--registry",
        str(registry),
        "--issue",
        "PGBOOT-004",
        "--branch",
        "ai/issue-4",
        "--file",
        "src/new.cpp",
    ]
    args[args.index(option) + 1] = blank_value

    result = run_claim_ticket(*args)

    assert result.returncode != 0
    assert "must be non-empty" in result.stderr
    assert read_registry(registry) == original


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


def test_rejects_absolute_owned_file_paths(tmp_path: Path):
    registry = tmp_path / "ownership.yaml"
    original = {"version": 1, "claims": []}
    write_registry(registry, original)

    result = run_claim_ticket(
        "--registry",
        str(registry),
        "--issue",
        "PGBOOT-007",
        "--branch",
        "ai/issue-7",
        "--file",
        "/src/hidden.cpp",
    )

    assert result.returncode != 0
    assert "relative to repository root" in result.stderr
    assert read_registry(registry) == original


def test_rejects_parent_escaping_owned_file_paths(tmp_path: Path):
    registry = tmp_path / "ownership.yaml"

    result = run_claim_ticket(
        "--registry",
        str(registry),
        "--issue",
        "PGBOOT-008",
        "--branch",
        "ai/issue-8",
        "--file",
        "../src/hidden.cpp",
    )

    assert result.returncode != 0
    assert "escape repository root" in result.stderr
    assert not registry.exists()


def test_rejects_malformed_registry_with_unsafe_owned_file(tmp_path: Path):
    cases = [
        ("absolute", "/src/hidden.cpp", "relative to repository root"),
        ("parent", "../src/hidden.cpp", "escape repository root"),
    ]
    for name, unsafe_path, expected_error in cases:
        registry = tmp_path / f"ownership-{name}.yaml"
        original = {
            "version": 1,
            "claims": [
                {
                    "issue": "PGBOOT-001",
                    "branch": "ai/issue-1",
                    "status": "active",
                    "owned_files": [unsafe_path],
                }
            ],
        }
        write_registry(registry, original)

        result = run_claim_ticket(
            "--registry",
            str(registry),
            "--issue",
            "PGBOOT-009",
            "--branch",
            "ai/issue-9",
            "--file",
            "src/new.cpp",
        )

        assert result.returncode != 0
        assert expected_error in result.stderr
        assert read_registry(registry) == original


def test_serializes_parallel_non_overlapping_claims(tmp_path: Path):
    registry = tmp_path / "ownership.yaml"
    claim_count = 30

    def claim(index: int) -> subprocess.CompletedProcess[str]:
        return run_claim_ticket(
            "--registry",
            str(registry),
            "--issue",
            f"PGBOOT-{index:03d}",
            "--branch",
            f"ai/issue-{index}",
            "--file",
            f"src/file_{index}.cpp",
        )

    with ThreadPoolExecutor(max_workers=12) as executor:
        results = list(executor.map(claim, range(claim_count)))

    failures = [result for result in results if result.returncode != 0]
    assert failures == []
    claims = read_registry(registry)["claims"]
    assert len(claims) == claim_count
    assert {claim["issue"] for claim in claims} == {
        f"PGBOOT-{index:03d}" for index in range(claim_count)
    }
    assert {claim["owned_files"][0] for claim in claims} == {
        f"src/file_{index}.cpp" for index in range(claim_count)
    }
