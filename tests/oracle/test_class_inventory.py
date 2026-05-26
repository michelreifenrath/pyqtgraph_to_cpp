from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

import pytest
import yaml

SCRIPT = Path("oracle/scripts/generate_class_inventory.py")
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


def write_fixture_file(path: Path, text: str) -> None:
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
    class NestedInClass:
        pass


def factory():
    class NestedInFunction:
        pass
    return NestedInFunction

VALUE = 1
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
    write_fixture_file(
        repo / "pyqtgraph" / "examples" / "Example.py",
        """class ExampleOnly:
    pass
""",
    )
    write_fixture_file(
        repo / "tests" / "test_x.py",
        """class TestOnly:
    pass
""",
    )
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


def make_inventory_root(tmp_path: Path) -> tuple[Path, str]:
    root = tmp_path / "workspace"
    checkout = root / CHECKOUT_PATH
    commit = populate_fixture_repo(checkout)
    write_source_lock(root, repo="fixture://pyqtgraph", commit=commit)
    return root, commit


def snapshot_tree(root: Path) -> dict[str, tuple[int, int]]:
    snapshot: dict[str, tuple[int, int]] = {}
    for path in sorted(p for p in root.rglob("*") if p.is_file()):
        stat = path.stat()
        snapshot[path.relative_to(root).as_posix()] = (stat.st_size, stat.st_mtime_ns)
    return snapshot


def expected_classes() -> list[dict[str, object]]:
    return [
        {
            "class_name": "PlotData",
            "upstream_path": "pyqtgraph/PlotData.py",
            "target_header_path": "include/pyqtgraph/PlotData.hpp",
            "target_source_path": "src/pyqtgraph/PlotData.cpp",
            "subsystem": "core",
            "bases": ["object"],
            "line": 1,
        },
        {
            "class_name": "HelperMixin",
            "upstream_path": "pyqtgraph/widgets/PlotWidget.py",
            "target_header_path": "include/pyqtgraph/widgets/PlotWidget.hpp",
            "target_source_path": "src/pyqtgraph/widgets/PlotWidget.cpp",
            "subsystem": "widgets",
            "bases": [],
            "line": 1,
        },
        {
            "class_name": "PlotWidget",
            "upstream_path": "pyqtgraph/widgets/PlotWidget.py",
            "target_header_path": "include/pyqtgraph/widgets/PlotWidget.hpp",
            "target_source_path": "src/pyqtgraph/widgets/PlotWidget.cpp",
            "subsystem": "widgets",
            "bases": ["HelperMixin", "QtWidgets.QWidget"],
            "line": 5,
        },
    ]


def expected_classes_with_status_metadata() -> list[dict[str, object]]:
    return [
        {**record, "status": "not_started", "completion": "missing"}
        for record in expected_classes()
    ]


def expected_generated_manifest_sections() -> dict[str, object]:
    return {
        "classes": expected_classes(),
        "excluded": {
            "examples": ["pyqtgraph/examples/Example.py"],
            "tests": ["tests/test_x.py"],
        },
        "summary": {
            "class_count": 3,
            "source_file_count": 2,
            "excluded_example_count": 1,
            "excluded_test_count": 1,
        },
    }


def test_help_exposes_class_inventory_cli_options() -> None:
    result = run_cli("--help")

    assert result.returncode == 0, result.stderr
    assert "--root" in result.stdout
    assert "--format" in result.stdout
    assert "--check" in result.stdout
    assert "--update-manifest" in result.stdout


def test_yaml_inventory_is_deterministic_sorted_and_maps_class_targets(
    tmp_path: Path,
) -> None:
    root, commit = make_inventory_root(tmp_path)

    first = run_cli("--root", str(root))
    second = run_cli("--root", str(root))

    assert first.returncode == 0, first.stderr
    assert second.returncode == 0, second.stderr
    assert first.stdout == second.stdout
    inventory = yaml.safe_load(first.stdout)

    assert inventory["reference"] == {
        "repo": "fixture://pyqtgraph",
        "ref": REF,
        "pinned_commit": commit,
        "docs_url": DOCS_URL,
        "checkout_path": CHECKOUT_PATH,
    }
    assert inventory["classes"] == expected_classes()
    assert [
        (r["upstream_path"], r["line"], r["class_name"]) for r in inventory["classes"]
    ] == sorted(
        (r["upstream_path"], r["line"], r["class_name"]) for r in inventory["classes"]
    )
    assert {r["class_name"] for r in inventory["classes"]}.isdisjoint(
        {"NestedInClass", "NestedInFunction", "ExampleOnly", "TestOnly"}
    )
    assert inventory["summary"] == {
        "class_count": 3,
        "source_file_count": 2,
        "excluded_example_count": 1,
        "excluded_test_count": 1,
    }


def test_json_inventory_reports_exclusions_and_matches_yaml_shape(
    tmp_path: Path,
) -> None:
    root, _commit = make_inventory_root(tmp_path)

    result = run_cli("--root", str(root), "--format", "json")

    assert result.returncode == 0, result.stderr
    inventory = json.loads(result.stdout)
    assert inventory["classes"] == expected_classes()
    assert inventory["excluded"] == {
        "examples": ["pyqtgraph/examples/Example.py"],
        "tests": ["tests/test_x.py"],
    }


def test_check_mode_validates_without_writes(tmp_path: Path) -> None:
    root, _commit = make_inventory_root(tmp_path)
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")

    assert result.returncode == 0, result.stderr
    assert "class inventory verified" in result.stdout
    assert "3 classes" in result.stdout
    assert snapshot_tree(root) == before


def test_check_mode_accepts_full_manifest_class_metadata_without_writes(
    tmp_path: Path,
) -> None:
    root, _commit = make_inventory_root(tmp_path)
    manifest_path = root / "port_manifest.yaml"
    manifest = {
        "reference": {"keep": "existing"},
        **expected_generated_manifest_sections(),
    }
    manifest["classes"] = expected_classes_with_status_metadata()
    manifest["summary"] = {
        **manifest["summary"],
        "example_count": 129,
        "example_asset_count": 16,
        "total_example_tree_file_count": 145,
    }
    manifest_path.write_text(
        yaml.safe_dump(manifest, sort_keys=False), encoding="utf-8"
    )
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")

    assert result.returncode == 0, result.stderr
    assert "class inventory verified" in result.stdout
    assert snapshot_tree(root) == before


def test_P0_02_check_mode_rejects_stale_class_metadata_without_writes(
    tmp_path: Path,
) -> None:
    root, _commit = make_inventory_root(tmp_path)
    manifest_path = root / "port_manifest.yaml"
    manifest = {
        "reference": {"keep": "existing"},
        **expected_generated_manifest_sections(),
    }
    manifest["classes"] = expected_classes_with_status_metadata()
    manifest["classes"][0]["status"] = "ported"
    manifest["classes"][0]["completion"] = "complete"
    manifest_path.write_text(
        yaml.safe_dump(manifest, sort_keys=False), encoding="utf-8"
    )
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")

    assert result.returncode != 0
    assert "port_manifest.yaml is stale" in result.stderr
    assert "classes" in result.stderr
    assert snapshot_tree(root) == before


def test_check_mode_rejects_stale_manifest_without_writes(tmp_path: Path) -> None:
    root, _commit = make_inventory_root(tmp_path)
    manifest_path = root / "port_manifest.yaml"
    manifest = {
        "reference": {"keep": "existing"},
        **expected_generated_manifest_sections(),
    }
    manifest["summary"] = {"class_count": 999}
    manifest_path.write_text(
        yaml.safe_dump(manifest, sort_keys=False), encoding="utf-8"
    )
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")

    assert result.returncode != 0
    assert "port_manifest.yaml is stale" in result.stderr
    assert "--update-manifest" in result.stderr
    assert "summary" in result.stderr
    assert snapshot_tree(root) == before


def test_check_mode_rejects_missing_manifest_section_without_writes(
    tmp_path: Path,
) -> None:
    root, _commit = make_inventory_root(tmp_path)
    manifest_path = root / "port_manifest.yaml"
    manifest = {
        "reference": {"keep": "existing"},
        **expected_generated_manifest_sections(),
    }
    del manifest["classes"]
    manifest_path.write_text(
        yaml.safe_dump(manifest, sort_keys=False), encoding="utf-8"
    )
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")

    assert result.returncode != 0
    assert "port_manifest.yaml is stale" in result.stderr
    assert "--update-manifest" in result.stderr
    assert "classes" in result.stderr
    assert snapshot_tree(root) == before


def test_check_mode_uses_read_only_fallback_when_checkout_absent(
    tmp_path: Path,
) -> None:
    upstream = tmp_path / "upstream"
    commit = populate_fixture_repo(upstream)
    root = tmp_path / "workspace"
    write_source_lock(root, repo=str(upstream), commit=commit)
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")

    assert result.returncode == 0, result.stderr
    assert "class inventory verified" in result.stdout
    assert "3 classes" in result.stdout
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


def test_ignored_local_python_files_do_not_contaminate_inventory(
    tmp_path: Path,
) -> None:
    root, _commit = make_inventory_root(tmp_path)
    checkout = root / CHECKOUT_PATH
    write_fixture_file(checkout / ".gitignore", "pyqtgraph/Ignored.py\n")
    git(checkout, "add", ".gitignore")
    git(checkout, "commit", "-m", "ignore local artifacts")
    commit = git(checkout, "rev-parse", "HEAD")
    write_source_lock(root, repo="fixture://pyqtgraph", commit=commit)
    write_fixture_file(
        checkout / "pyqtgraph" / "Ignored.py", "class IgnoredArtifact:\n    pass\n"
    )

    result = run_cli("--root", str(root))

    assert result.returncode == 0, result.stderr
    inventory = yaml.safe_load(result.stdout)
    assert {record["class_name"] for record in inventory["classes"]} == {
        "PlotData",
        "HelperMixin",
        "PlotWidget",
    }


@pytest.mark.parametrize("dirty_state", ["untracked", "modified", "deleted"])
def test_rejects_dirty_local_checkout_at_pinned_commit(
    tmp_path: Path,
    dirty_state: str,
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


def test_update_manifest_replaces_generated_sections_and_is_idempotent(
    tmp_path: Path,
) -> None:
    root, _commit = make_inventory_root(tmp_path)
    manifest_path = root / "port_manifest.yaml"
    manifest_path.write_text(
        yaml.safe_dump(
            {
                "reference": {"keep": "existing"},
                "notes": ["preserve unrelated sections"],
                "classes": [{"class_name": "Stale"}],
                "summary": {"class_count": 999, "example_count": 10},
                "excluded": {"examples": ["stale.py"], "tests": []},
            },
            sort_keys=False,
        ),
        encoding="utf-8",
    )
    before_files = {
        p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file()
    }

    result = run_cli("--root", str(root), "--update-manifest")

    assert result.returncode == 0, result.stderr
    assert result.stdout == "updated port_manifest.yaml\n"
    after_first = manifest_path.read_text(encoding="utf-8")
    manifest = yaml.safe_load(after_first)
    assert manifest["reference"] == {"keep": "existing"}
    assert manifest["notes"] == ["preserve unrelated sections"]
    assert manifest["classes"] == expected_classes_with_status_metadata()
    assert manifest["excluded"] == {
        "examples": ["pyqtgraph/examples/Example.py"],
        "tests": ["tests/test_x.py"],
    }
    assert manifest["summary"] == {
        "class_count": 3,
        "source_file_count": 2,
        "excluded_example_count": 1,
        "excluded_test_count": 1,
        "example_count": 10,
    }
    assert {
        p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file()
    } == before_files

    second = run_cli("--root", str(root), "--update-manifest")

    assert second.returncode == 0, second.stderr
    assert second.stdout == "updated port_manifest.yaml\n"
    assert manifest_path.read_text(encoding="utf-8") == after_first


def test_check_and_update_manifest_are_mutually_exclusive() -> None:
    result = run_cli("--check", "--update-manifest")

    assert result.returncode != 0
    assert "not allowed with argument" in result.stderr
