from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path
from typing import Any

import pytest
import yaml

SCRIPT = Path("scripts/generate_manifest")
REF = "pyqtgraph-0.14.0"
DOCS_URL = "https://pyqtgraph.readthedocs.io/"
CHECKOUT_PATH = "reference/pyqtgraph"


def run_cli(*args: str, root: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(SCRIPT.resolve()), *args],
        cwd=root,
        text=True,
        capture_output=True,
    )


def git(repo: Path, *args: str) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=repo,
        check=True,
        text=True,
        capture_output=True,
    )
    return result.stdout.strip()


def write_fixture_file(path: Path, text: str = "# fixture\n") -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def populate_fixture_repo(repo: Path) -> str:
    repo.mkdir(parents=True)
    git(repo, "init")
    git(repo, "config", "user.email", "test@example.invalid")
    git(repo, "config", "user.name", "Test User")
    write_fixture_file(
        repo / "pyqtgraph" / "PlotData.py",
        """class PlotData(object):
    pass
""",
    )
    write_fixture_file(
        repo / "pyqtgraph" / "widgets" / "PlotWidget.py",
        """class HelperMixin:
    pass


class PlotWidget(HelperMixin, QtWidgets.QWidget):
    pass
""",
    )
    write_fixture_file(repo / "pyqtgraph" / "examples" / "Example.py")
    write_fixture_file(repo / "pyqtgraph" / "examples" / "nested" / "Advanced.py")
    write_fixture_file(
        repo / "pyqtgraph" / "examples" / "designerExample.ui", "<ui/>\n"
    )
    write_fixture_file(repo / "tests" / "test_x.py")
    git(repo, "add", ".")
    git(repo, "commit", "-m", "fixture")
    git(repo, "tag", REF)
    return git(repo, "rev-parse", "HEAD")


def write_source_lock(root: Path, *, repo: str, commit: str) -> None:
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


def write_fixture_targets(root: Path) -> None:
    write_fixture_file(root / "include" / "cppqtgraph" / "PlotData.hpp")
    write_fixture_file(root / "src" / "cppqtgraph" / "PlotData.cpp")
    write_fixture_file(root / "include" / "cppqtgraph" / "widgets" / "PlotWidget.hpp")
    write_fixture_file(root / "examples" / "Example.cpp")


def make_inventory_root(tmp_path: Path) -> tuple[Path, str]:
    root = tmp_path / "workspace"
    checkout = root / CHECKOUT_PATH
    commit = populate_fixture_repo(checkout)
    write_source_lock(root, repo="fixture://pyqtgraph", commit=commit)
    write_fixture_targets(root)
    return root, commit


def snapshot_tree(root: Path) -> dict[str, tuple[int, int]]:
    snapshot: dict[str, tuple[int, int]] = {}
    for path in sorted(p for p in root.rglob("*") if p.is_file()):
        stat = path.stat()
        snapshot[path.relative_to(root).as_posix()] = (stat.st_size, stat.st_mtime_ns)
    return snapshot


def expected_source_files() -> list[dict[str, str]]:
    return [
        {
            "upstream_path": "pyqtgraph/PlotData.py",
            "target_header_path": "include/cppqtgraph/PlotData.hpp",
            "target_source_path": "src/cppqtgraph/PlotData.cpp",
            "subsystem": "core",
            "status": "ported",
            "completion": "missing",
            "target_presence": "all",
        },
        {
            "upstream_path": "pyqtgraph/widgets/PlotWidget.py",
            "target_header_path": "include/cppqtgraph/widgets/PlotWidget.hpp",
            "target_source_path": "src/cppqtgraph/widgets/PlotWidget.cpp",
            "subsystem": "widgets",
            "status": "partial",
            "completion": "missing",
            "target_presence": "some",
        },
    ]


def expected_classes() -> list[dict[str, object]]:
    return [
        {
            "class_name": "PlotData",
            "upstream_path": "pyqtgraph/PlotData.py",
            "target_header_path": "include/cppqtgraph/PlotData.hpp",
            "target_source_path": "src/cppqtgraph/PlotData.cpp",
            "subsystem": "core",
            "bases": ["object"],
            "line": 1,
            "status": "ported",
            "completion": "missing",
            "target_presence": "all",
        },
        {
            "class_name": "HelperMixin",
            "upstream_path": "pyqtgraph/widgets/PlotWidget.py",
            "target_header_path": "include/cppqtgraph/widgets/PlotWidget.hpp",
            "target_source_path": "src/cppqtgraph/widgets/PlotWidget.cpp",
            "subsystem": "widgets",
            "bases": [],
            "line": 1,
            "status": "partial",
            "completion": "missing",
            "target_presence": "some",
        },
        {
            "class_name": "PlotWidget",
            "upstream_path": "pyqtgraph/widgets/PlotWidget.py",
            "target_header_path": "include/cppqtgraph/widgets/PlotWidget.hpp",
            "target_source_path": "src/cppqtgraph/widgets/PlotWidget.cpp",
            "subsystem": "widgets",
            "bases": ["HelperMixin", "QtWidgets.QWidget"],
            "line": 5,
            "status": "partial",
            "completion": "missing",
            "target_presence": "some",
        },
    ]


def expected_manifest(commit: str) -> dict[str, Any]:
    return {
        "reference": {
            "repo": "fixture://pyqtgraph",
            "ref": REF,
            "pinned_commit": commit,
            "docs_url": DOCS_URL,
        },
        "manifest_schema": {"status_metadata": "evidence_backed"},
        "source_files": expected_source_files(),
        "examples": [
            {
                "upstream_path": "pyqtgraph/examples/Example.py",
                "target_source_path": "examples/Example.cpp",
                "name": "Example",
                "category": "root",
                "status": "ported",
                "completion": "missing",
                "target_presence": "all",
            },
            {
                "upstream_path": "pyqtgraph/examples/nested/Advanced.py",
                "target_source_path": "examples/nested/Advanced.cpp",
                "name": "nested/Advanced",
                "category": "nested",
                "status": "not_started",
                "completion": "missing",
                "target_presence": "none",
            },
        ],
        "example_assets": [
            {
                "upstream_path": "pyqtgraph/examples/designerExample.ui",
                "target_path": "examples/designerExample.ui",
                "status": "not_started",
                "completion": "missing",
                "target_presence": "none",
            }
        ],
        "example_inventory_summary": {
            "example_count": 2,
            "asset_count": 1,
            "total_example_tree_file_count": 3,
        },
        "classes": expected_classes(),
        "excluded": {
            "examples": [
                "pyqtgraph/examples/Example.py",
                "pyqtgraph/examples/nested/Advanced.py",
            ],
            "tests": ["tests/test_x.py"],
        },
        "summary": {
            "source_file_count": 2,
            "example_count": 2,
            "example_asset_count": 1,
            "total_example_tree_file_count": 3,
            "class_count": 3,
            "excluded_example_count": 2,
            "excluded_test_count": 1,
        },
    }


def strip_manifest_status_metadata(manifest: dict[str, Any]) -> dict[str, Any]:
    stripped = dict(manifest)
    for section in ("source_files", "examples", "example_assets", "classes"):
        stripped[section] = [
            {
                key: value
                for key, value in row.items()
                if key not in ("status", "completion", "target_presence", "completion_evidence")
            }
            for row in manifest[section]
        ]
    return stripped


def test_help_exposes_manifest_cli_options() -> None:
    result = run_cli("--help")

    assert result.returncode == 0, result.stderr
    assert "--root" in result.stdout
    assert "--format" in result.stdout
    assert "--check" in result.stdout
    assert "--update-manifest" in result.stdout


def test_yaml_manifest_is_deterministic_sorted_and_uses_canonical_schema(
    tmp_path: Path,
) -> None:
    root, commit = make_inventory_root(tmp_path)

    first = run_cli("--root", str(root))
    second = run_cli("--root", str(root))

    assert first.returncode == 0, first.stderr
    assert second.returncode == 0, second.stderr
    assert first.stdout == second.stdout
    manifest = yaml.safe_load(first.stdout)
    assert list(manifest) == [
        "reference",
        "manifest_schema",
        "source_files",
        "examples",
        "example_assets",
        "example_inventory_summary",
        "classes",
        "excluded",
        "summary",
    ]
    assert manifest == expected_manifest(commit)
    for section in ("source_files", "examples", "example_assets", "classes"):
        for row in manifest[section]:
            assert "status" in row
            assert "completion" in row
    assert "assets" not in manifest


def test_existing_targets_remain_incomplete_without_completion_evidence(
    tmp_path: Path,
) -> None:
    root, _commit = make_inventory_root(tmp_path)

    result = run_cli("--root", str(root), "--format", "json")

    assert result.returncode == 0, result.stderr
    manifest = json.loads(result.stdout)
    assert manifest["source_files"][0]["target_presence"] == "all"
    assert manifest["source_files"][0]["status"] == "ported"
    assert manifest["source_files"][0]["completion"] == "missing"
    assert manifest["classes"][0]["target_presence"] == "all"
    assert manifest["classes"][0]["status"] == "ported"
    assert manifest["classes"][0]["completion"] == "missing"


def test_completion_evidence_marks_only_matching_row_complete(tmp_path: Path) -> None:
    root, _commit = make_inventory_root(tmp_path)
    write_fixture_file(root / "tests" / "oracle" / "plotdata-evidence.txt", "passed\n")
    (root / "port_manifest.yaml").write_text(
        yaml.safe_dump(
            {
                "completion_evidence": [
                    {
                        "section": "source_files",
                        "upstream_path": "pyqtgraph/PlotData.py",
                        "evidence_type": "focused_test",
                        "artifact_path": "tests/oracle/plotdata-evidence.txt",
                    }
                ]
            },
            sort_keys=False,
        ),
        encoding="utf-8",
    )

    result = run_cli("--root", str(root), "--format", "json")

    assert result.returncode == 0, result.stderr
    manifest = json.loads(result.stdout)
    assert manifest["source_files"][0]["completion"] == "complete"
    assert manifest["source_files"][0]["completion_evidence"] == {
        "type": "focused_test",
        "artifact_path": "tests/oracle/plotdata-evidence.txt",
    }
    assert manifest["classes"][0]["completion"] == "missing"


def test_completion_evidence_rejects_unknown_manifest_row(tmp_path: Path) -> None:
    root, _commit = make_inventory_root(tmp_path)
    write_fixture_file(root / "tests" / "oracle" / "plotdata-evidence.txt", "passed\n")
    (root / "port_manifest.yaml").write_text(
        yaml.safe_dump(
            {
                "completion_evidence": [
                    {
                        "section": "source_files",
                        "upstream_path": "pyqtgraph/Typo.py",
                        "evidence_type": "focused_test",
                        "artifact_path": "tests/oracle/plotdata-evidence.txt",
                    }
                ]
            },
            sort_keys=False,
        ),
        encoding="utf-8",
    )

    result = run_cli("--root", str(root), "--format", "json")

    assert result.returncode == 1
    assert "completion_evidence references unknown manifest row" in result.stderr
    assert "pyqtgraph/Typo.py" in result.stderr


def test_json_manifest_matches_yaml_shape(tmp_path: Path) -> None:
    root, commit = make_inventory_root(tmp_path)

    result = run_cli("--root", str(root), "--format", "json")

    assert result.returncode == 0, result.stderr
    assert json.loads(result.stdout) == expected_manifest(commit)


def test_update_manifest_replaces_generated_sections_and_is_idempotent(
    tmp_path: Path,
) -> None:
    root, commit = make_inventory_root(tmp_path)
    manifest_path = root / "port_manifest.yaml"
    manifest_path.write_text(
        yaml.safe_dump(
            {
                "notes": ["preserve unrelated sections"],
                "source_files": [{"upstream_path": "stale.py"}],
                "summary": {"source_file_count": 999},
            },
            sort_keys=False,
        ),
        encoding="utf-8",
    )

    result = run_cli("--root", str(root), "--update-manifest")

    assert result.returncode == 0, result.stderr
    assert result.stdout == "updated port_manifest.yaml\n"
    after_first = manifest_path.read_text(encoding="utf-8")
    manifest = yaml.safe_load(after_first)
    expected = expected_manifest(commit)
    assert {key: manifest[key] for key in expected} == expected
    assert manifest["notes"] == ["preserve unrelated sections"]

    second = run_cli("--root", str(root), "--update-manifest")

    assert second.returncode == 0, second.stderr
    assert manifest_path.read_text(encoding="utf-8") == after_first


def test_check_mode_validates_manifest_without_writes(tmp_path: Path) -> None:
    root, _commit = make_inventory_root(tmp_path)
    assert run_cli("--root", str(root), "--update-manifest").returncode == 0
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")

    assert result.returncode == 0, result.stderr
    assert "port manifest verified" in result.stdout
    assert snapshot_tree(root) == before


@pytest.mark.parametrize("bad_manifest", ["missing", "stale"])
def test_check_mode_rejects_missing_or_stale_manifest_without_writes(
    tmp_path: Path, bad_manifest: str
) -> None:
    root, _commit = make_inventory_root(tmp_path)
    if bad_manifest == "stale":
        (root / "port_manifest.yaml").write_text(
            "summary:\n  source_file_count: 999\n", encoding="utf-8"
        )
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")

    assert result.returncode != 0
    assert "port_manifest.yaml" in result.stderr
    assert "--update-manifest" in result.stderr
    assert snapshot_tree(root) == before


def test_P0_02_check_mode_rejects_deferred_manifest_without_row_metadata(
    tmp_path: Path,
) -> None:
    root, commit = make_inventory_root(tmp_path)
    manifest_path = root / "port_manifest.yaml"
    manifest = strip_manifest_status_metadata(expected_manifest(commit))
    manifest["manifest_schema"] = {"status_metadata": "deferred"}
    manifest_path.write_text(
        yaml.safe_dump(manifest, sort_keys=False),
        encoding="utf-8",
    )
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")
    generated = run_cli("--root", str(root))

    assert result.returncode != 0
    assert "port_manifest.yaml" in result.stderr
    assert "status" in result.stderr
    assert "--update-manifest" in result.stderr
    assert generated.returncode == 0, generated.stderr
    generated_manifest = yaml.safe_load(generated.stdout)
    assert generated_manifest["manifest_schema"] == {"status_metadata": "evidence_backed"}
    for section in ("source_files", "examples", "example_assets", "classes"):
        for row in generated_manifest[section]:
            assert "status" in row
            assert "completion" in row
    assert snapshot_tree(root) == before


@pytest.mark.parametrize(
    "metadata_fixture", ["one_missing_status", "all_metadata_stripped"]
)
def test_P0_02_check_mode_rejects_missing_or_stale_status_metadata(
    tmp_path: Path, metadata_fixture: str
) -> None:
    root, _commit = make_inventory_root(tmp_path)
    assert run_cli("--root", str(root), "--update-manifest").returncode == 0
    manifest_path = root / "port_manifest.yaml"
    manifest = yaml.safe_load(manifest_path.read_text(encoding="utf-8"))
    if metadata_fixture == "one_missing_status":
        if "status" in manifest["source_files"][0]:
            del manifest["source_files"][0]["status"]
    else:
        manifest = strip_manifest_status_metadata(manifest)
    manifest_path.write_text(
        yaml.safe_dump(manifest, sort_keys=False), encoding="utf-8"
    )
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")

    assert result.returncode != 0
    assert "port_manifest.yaml" in result.stderr
    assert "status" in result.stderr
    assert "--update-manifest" in result.stderr
    assert snapshot_tree(root) == before


def test_check_mode_uses_read_only_fallback_when_checkout_absent(
    tmp_path: Path,
) -> None:
    upstream = tmp_path / "upstream"
    commit = populate_fixture_repo(upstream)
    root = tmp_path / "workspace"
    write_source_lock(root, repo=str(upstream), commit=commit)
    write_fixture_targets(root)
    manifest = expected_manifest(commit)
    reference = dict(manifest["reference"])
    reference["repo"] = str(upstream)
    manifest["reference"] = reference
    (root / "port_manifest.yaml").write_text(
        yaml.safe_dump(manifest, sort_keys=False), encoding="utf-8"
    )
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")

    assert result.returncode == 0, result.stderr
    assert "port manifest verified" in result.stdout
    assert not (root / CHECKOUT_PATH).exists()
    assert snapshot_tree(root) == before


def test_rejects_checkout_at_wrong_commit(tmp_path: Path) -> None:
    root, _commit = make_inventory_root(tmp_path)
    lock_path = root / "reference" / "source.lock"
    lock = yaml.safe_load(lock_path.read_text(encoding="utf-8"))
    lock["pinned_commit"] = "0" * 40
    lock_path.write_text(yaml.safe_dump(lock, sort_keys=False), encoding="utf-8")

    result = run_cli("--root", str(root))

    assert result.returncode != 0
    assert "pinned_commit" in result.stderr


@pytest.mark.parametrize("dirty_state", ["untracked", "modified", "deleted"])
def test_rejects_dirty_local_checkout_at_pinned_commit(
    tmp_path: Path, dirty_state: str
) -> None:
    root, commit = make_inventory_root(tmp_path)
    checkout = root / CHECKOUT_PATH
    assert git(checkout, "rev-parse", "HEAD") == commit
    if dirty_state == "untracked":
        write_fixture_file(checkout / "pyqtgraph" / "Bogus.py", "class Bogus: pass\n")
    elif dirty_state == "modified":
        write_fixture_file(checkout / "pyqtgraph" / "PlotData.py", "# changed\n")
    else:
        (checkout / "pyqtgraph" / "PlotData.py").unlink()

    result = run_cli("--root", str(root))

    assert result.returncode != 0
    assert "must be clean" in result.stderr
    assert "non-deterministic" in result.stderr


def test_check_and_update_manifest_are_mutually_exclusive() -> None:
    result = run_cli("--check", "--update-manifest")

    assert result.returncode != 0
    assert "not allowed with argument" in result.stderr
