from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

import pytest
import yaml

SCRIPT = Path("oracle/scripts/generate_source_inventory.py")
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

    write_fixture_file(repo / "pyqtgraph" / "widgets" / "PlotWidget.py")
    write_fixture_file(repo / "pyqtgraph" / "PlotData.py")
    write_fixture_file(repo / "pyqtgraph" / "examples" / "Example.py")
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


def test_help_exposes_inventory_cli_options() -> None:
    result = run_cli("--help")

    assert result.returncode == 0, result.stderr
    assert "--root" in result.stdout
    assert "--format" in result.stdout
    assert "--check" in result.stdout


def test_yaml_inventory_is_deterministic_sorted_and_maps_targets(
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
    assert inventory["source_files"] == [
        {
            "upstream_path": "pyqtgraph/PlotData.py",
            "target_header_path": "include/pyqtgraph/PlotData.hpp",
            "target_source_path": "src/pyqtgraph/PlotData.cpp",
            "subsystem": "core",
        },
        {
            "upstream_path": "pyqtgraph/widgets/PlotWidget.py",
            "target_header_path": "include/pyqtgraph/widgets/PlotWidget.hpp",
            "target_source_path": "src/pyqtgraph/widgets/PlotWidget.cpp",
            "subsystem": "widgets",
        },
    ]
    assert [r["upstream_path"] for r in inventory["source_files"]] == sorted(
        r["upstream_path"] for r in inventory["source_files"]
    )
    assert inventory["summary"] == {
        "source_file_count": 2,
        "excluded_example_count": 1,
        "excluded_test_count": 1,
    }


def test_excludes_examples_and_tests_but_reports_them(tmp_path: Path) -> None:
    root, _commit = make_inventory_root(tmp_path)

    result = run_cli("--root", str(root), "--format", "json")

    assert result.returncode == 0, result.stderr
    inventory = json.loads(result.stdout)
    upstream_paths = {record["upstream_path"] for record in inventory["source_files"]}
    assert "pyqtgraph/examples/Example.py" not in upstream_paths
    assert "tests/test_x.py" not in upstream_paths
    assert inventory["excluded"] == {
        "examples": ["pyqtgraph/examples/Example.py"],
        "tests": ["tests/test_x.py"],
    }


def test_check_mode_validates_without_writes(tmp_path: Path) -> None:
    root, _commit = make_inventory_root(tmp_path)
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")

    assert result.returncode == 0, result.stderr
    assert "source inventory verified" in result.stdout
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
    assert "source inventory verified" in result.stdout
    assert "2 source files" in result.stdout
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
    tmp_path: Path,
    dirty_state: str,
) -> None:
    root, commit = make_inventory_root(tmp_path)
    checkout = root / CHECKOUT_PATH
    assert git(checkout, "rev-parse", "HEAD") == commit

    if dirty_state == "untracked":
        write_fixture_file(checkout / "pyqtgraph" / "Bogus.py")
    elif dirty_state == "modified":
        write_fixture_file(checkout / "pyqtgraph" / "PlotData.py", "# changed\n")
    else:
        (checkout / "pyqtgraph" / "PlotData.py").unlink()

    result = run_cli("--root", str(root))

    assert result.returncode != 0
    assert "must be clean" in result.stderr
    assert "non-deterministic" in result.stderr
