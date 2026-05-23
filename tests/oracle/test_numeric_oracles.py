from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path
from typing import Any

import yaml

SCRIPT = Path("oracle/scripts/generate_numeric_oracles.py")
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


def write_source_lock(
    root: Path, repo: str = "fixture://pyqtgraph", commit: str = COMMIT
) -> None:
    (root / "reference").mkdir(parents=True, exist_ok=True)
    (root / "reference" / "source.lock").write_text(
        yaml.safe_dump(
            {
                "repo": repo,
                "ref": REF,
                "pinned_commit": commit,
                "docs_url": DOCS_URL,
                "checkout_path": CHECKOUT_PATH,
            },
            sort_keys=False,
        ),
        encoding="utf-8",
    )


def snapshot_tree(root: Path) -> dict[str, tuple[int, int]]:
    snapshot: dict[str, tuple[int, int]] = {}
    for path in sorted(p for p in root.rglob("*") if p.is_file()):
        stat = path.stat()
        snapshot[path.relative_to(root).as_posix()] = (stat.st_size, stat.st_mtime_ns)
    return snapshot


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def test_help_exposes_numeric_oracle_cli_options() -> None:
    result = run_cli("--help")

    assert result.returncode == 0, result.stderr
    assert "--root" in result.stdout
    assert "--format" in result.stdout
    assert "--check" in result.stdout


def test_yaml_manifest_is_deterministic_and_has_stable_schema(tmp_path: Path) -> None:
    root = tmp_path / "workspace"
    write_source_lock(root)

    first = run_cli("--root", str(root))
    second = run_cli("--root", str(root))

    assert first.returncode == 0, first.stderr
    assert second.returncode == 0, second.stderr
    assert first.stdout == second.stdout
    manifest = yaml.safe_load(first.stdout)

    assert list(manifest) == ["reference", "cases", "summary"]
    assert manifest["reference"] == {
        "repo": "fixture://pyqtgraph",
        "ref": REF,
        "pinned_commit": COMMIT,
        "docs_url": DOCS_URL,
        "checkout_path": CHECKOUT_PATH,
    }
    assert manifest["summary"] == {"case_count": 2}
    assert [case["id"] for case in manifest["cases"]] == [
        "affine_transform",
        "log_mapping",
    ]


def test_json_manifest_is_valid_and_sorted(tmp_path: Path) -> None:
    root = tmp_path / "workspace"
    write_source_lock(root)

    result = run_cli("--root", str(root), "--format", "json")

    assert result.returncode == 0, result.stderr
    manifest = json.loads(result.stdout)
    assert list(manifest) == ["reference", "cases", "summary"]
    case_ids = [case["id"] for case in manifest["cases"]]
    assert case_ids == sorted(case_ids)


def test_normal_mode_writes_deterministic_numeric_fixtures(tmp_path: Path) -> None:
    root = tmp_path / "workspace"
    write_source_lock(root)

    first = run_cli("--root", str(root), "--format", "json")
    fixtures_after_first = snapshot_tree(root / "oracle" / "fixtures" / "numeric")
    second = run_cli("--root", str(root), "--format", "json")

    assert first.returncode == 0, first.stderr
    assert second.returncode == 0, second.stderr
    assert first.stdout == second.stdout
    assert (
        snapshot_tree(root / "oracle" / "fixtures" / "numeric") == fixtures_after_first
    )

    fixture_dir = root / "oracle" / "fixtures" / "numeric"
    affine_path = fixture_dir / "affine_transform.json"
    log_path = fixture_dir / "log_mapping.json"
    assert affine_path.is_file()
    assert log_path.is_file()

    affine = load_json(affine_path)
    log_mapping = load_json(log_path)
    assert list(affine) == [
        "schema_version",
        "case",
        "reference",
        "inputs",
        "expected",
        "tolerance",
    ]
    assert affine["schema_version"] == 1
    assert affine["case"] == "affine_transform"
    assert affine["reference"] == {"ref": REF, "pinned_commit": COMMIT}
    assert affine["expected"]["points"] == [[1.5, -2.0], [3.5, 4.0], [-4.5, 11.5]]
    assert affine["tolerance"] == {"absolute": 0.0, "relative": 0.0}

    assert log_mapping["schema_version"] == 1
    assert log_mapping["case"] == "log_mapping"
    assert log_mapping["expected"]["values"] == [-1.0, 0.0, 1.0, 2.0]
    assert log_mapping["tolerance"] == {"absolute": 1.0e-12, "relative": 1.0e-12}


def test_check_mode_validates_without_writes(tmp_path: Path) -> None:
    root = tmp_path / "workspace"
    write_source_lock(root)
    write_result = run_cli("--root", str(root))
    assert write_result.returncode == 0, write_result.stderr
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")

    assert result.returncode == 0, result.stderr
    assert "numeric oracles verified" in result.stdout
    assert snapshot_tree(root) == before


def test_rejects_invalid_or_missing_reference_lock(tmp_path: Path) -> None:
    missing_root = tmp_path / "missing"

    missing = run_cli("--root", str(missing_root))

    assert missing.returncode != 0
    assert missing.stderr.startswith("error:")

    invalid_root = tmp_path / "invalid"
    (invalid_root / "reference").mkdir(parents=True)
    (invalid_root / "reference" / "source.lock").write_text(
        "ref: only-ref\n", encoding="utf-8"
    )

    invalid = run_cli("--root", str(invalid_root))

    assert invalid.returncode != 0
    assert invalid.stderr.startswith("error:")


def test_check_mode_requires_fixtures_without_writes(tmp_path: Path) -> None:
    root = tmp_path / "workspace"
    write_source_lock(root)
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")

    assert result.returncode != 0
    assert result.stderr.startswith("error:")
    assert "missing numeric fixture" in result.stderr
    assert snapshot_tree(root) == before
    assert not (root / "oracle" / "fixtures" / "numeric").exists()


def test_check_mode_rejects_missing_expected_fixture(tmp_path: Path) -> None:
    root = tmp_path / "workspace"
    write_source_lock(root)
    write_result = run_cli("--root", str(root))
    assert write_result.returncode == 0, write_result.stderr
    missing_fixture = root / "oracle" / "fixtures" / "numeric" / "log_mapping.json"
    missing_fixture.unlink()

    result = run_cli("--root", str(root), "--check")

    assert result.returncode != 0
    assert result.stderr.startswith("error:")
    assert "missing numeric fixture" in result.stderr
    assert "log_mapping.json" in result.stderr


def test_check_mode_rejects_stale_existing_fixture(tmp_path: Path) -> None:
    root = tmp_path / "workspace"
    write_source_lock(root)
    write_result = run_cli("--root", str(root))
    assert write_result.returncode == 0, write_result.stderr
    fixture = root / "oracle" / "fixtures" / "numeric" / "affine_transform.json"
    data = load_json(fixture)
    data["expected"]["points"][0] = [99.0, 99.0]
    fixture.write_text(
        json.dumps(data, indent=2, sort_keys=False) + "\n", encoding="utf-8"
    )

    result = run_cli("--root", str(root), "--check")

    assert result.returncode != 0
    assert result.stderr.startswith("error:")
    assert "stale" in result.stderr
