from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path

import yaml

REPO = "https://github.com/pyqtgraph/pyqtgraph"
REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"
DOCS_URL = "https://pyqtgraph.readthedocs.io/"
CHECKOUT_PATH = "reference/pyqtgraph"
SCRIPT = Path("scripts/bootstrap_reference")


def load_yaml(path: Path):
    return yaml.safe_load(path.read_text(encoding="utf-8"))


def test_reference_metadata_files_agree_on_pin():
    lock = load_yaml(Path("reference/source.lock"))
    ref_text = Path("reference/PYQTGRAPH_REF").read_text(encoding="utf-8")
    manifest = load_yaml(Path("examples/example_manifest.yaml"))

    expected = {
        "repo": REPO,
        "ref": REF,
        "pinned_commit": PINNED_COMMIT,
        "docs_url": DOCS_URL,
        "checkout_path": CHECKOUT_PATH,
    }
    assert lock == expected
    assert manifest["reference"] == {
        "repo": REPO,
        "ref": REF,
        "pinned_commit": PINNED_COMMIT,
        "docs_url": DOCS_URL,
    }
    for key, value in expected.items():
        assert f"{key}: {value}" in ref_text


def test_check_offline_verifies_reference_without_writes():
    tracked_paths = [
        Path("reference/source.lock"),
        Path("reference/PYQTGRAPH_REF"),
        Path("examples/example_manifest.yaml"),
    ]
    before = {path: path.read_bytes() for path in tracked_paths}

    result = subprocess.run(
        [sys.executable, str(SCRIPT), "--check", "--offline"],
        text=True,
        capture_output=True,
    )

    assert result.returncode == 0, result.stderr
    assert "PyQtGraph reference verified" in result.stdout
    assert {path: path.read_bytes() for path in tracked_paths} == before


def copy_metadata(root: Path) -> None:
    (root / "reference").mkdir(parents=True)
    shutil.copy2("reference/source.lock", root / "reference/source.lock")
    shutil.copy2("reference/PYQTGRAPH_REF", root / "reference/PYQTGRAPH_REF")
    (root / "examples").mkdir(parents=True, exist_ok=True)
    shutil.copy2("examples/example_manifest.yaml", root / "examples/example_manifest.yaml")


def test_check_offline_rejects_unverifiable_checkout_directory(tmp_path: Path):
    copy_metadata(tmp_path)
    checkout = tmp_path / CHECKOUT_PATH
    checkout.mkdir()
    (checkout / "README.md").write_text("copied pyqtgraph tree\n", encoding="utf-8")

    result = subprocess.run(
        [
            sys.executable,
            str(SCRIPT.resolve()),
            "--check",
            "--offline",
            "--root",
            str(tmp_path),
        ],
        text=True,
        capture_output=True,
    )

    assert result.returncode != 0
    assert "not a git repository" in result.stderr


def test_check_offline_reports_mismatched_example_manifest_field(tmp_path: Path):
    copy_metadata(tmp_path)
    manifest_path = tmp_path / "examples" / "example_manifest.yaml"
    manifest = load_yaml(manifest_path)
    manifest["reference"]["pinned_commit"] = "0" * 40
    manifest_path.write_text(
        yaml.safe_dump(manifest, sort_keys=False), encoding="utf-8"
    )

    result = subprocess.run(
        [
            sys.executable,
            str(SCRIPT.resolve()),
            "--check",
            "--offline",
            "--root",
            str(tmp_path),
        ],
        text=True,
        capture_output=True,
    )

    assert result.returncode != 0
    assert "reference.pinned_commit" in result.stderr


def git(repo: Path, *args: str) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=repo,
        check=True,
        text=True,
        capture_output=True,
    )
    return result.stdout.strip()


def make_local_pyqtgraph_repo(
    tmp_path: Path, name: str = "remote", readme: str = "local pyqtgraph fixture\n"
) -> tuple[Path, str]:
    repo = tmp_path / name
    repo.mkdir()
    git(repo, "init")
    git(repo, "config", "user.email", "test@example.invalid")
    git(repo, "config", "user.name", "Test User")
    (repo / "README.md").write_text(readme, encoding="utf-8")
    git(repo, "add", "README.md")
    git(repo, "commit", "-m", "fixture")
    git(repo, "tag", REF)
    return repo, git(repo, "rev-parse", "HEAD")


def test_refresh_with_local_git_remote_writes_checkout_and_metadata(tmp_path: Path):
    remote, commit = make_local_pyqtgraph_repo(tmp_path)
    root = tmp_path / "workspace"
    root.mkdir()

    result = subprocess.run(
        [
            sys.executable,
            str(SCRIPT.resolve()),
            "--refresh",
            "--root",
            str(root),
            "--repo",
            str(remote),
        ],
        text=True,
        capture_output=True,
    )

    assert result.returncode == 0, result.stderr
    assert "refreshed PyQtGraph reference" in result.stdout
    assert git(root / CHECKOUT_PATH, "rev-parse", "HEAD") == commit

    lock = load_yaml(root / "reference/source.lock")
    assert lock == {
        "repo": str(remote),
        "ref": REF,
        "pinned_commit": commit,
        "docs_url": DOCS_URL,
        "checkout_path": CHECKOUT_PATH,
    }
    assert load_yaml(root / "examples" / "example_manifest.yaml")["reference"] == {
        "repo": str(remote),
        "ref": REF,
        "pinned_commit": commit,
        "docs_url": DOCS_URL,
    }
    ref_text = (root / "reference/PYQTGRAPH_REF").read_text(encoding="utf-8")
    assert f"pinned_commit: {commit}" in ref_text

    check = subprocess.run(
        [
            sys.executable,
            str(SCRIPT.resolve()),
            "--check",
            "--offline",
            "--root",
            str(root),
            "--repo",
            str(remote),
        ],
        text=True,
        capture_output=True,
    )
    assert check.returncode == 0, check.stderr


def test_refresh_ignores_nested_checkout_in_parent_repo(tmp_path: Path):
    remote, _commit = make_local_pyqtgraph_repo(tmp_path)
    root = tmp_path / "workspace"
    root.mkdir()
    git(root, "init")

    result = subprocess.run(
        [
            sys.executable,
            str(SCRIPT.resolve()),
            "--refresh",
            "--root",
            str(root),
            "--repo",
            str(remote),
        ],
        text=True,
        capture_output=True,
    )

    assert result.returncode == 0, result.stderr
    exclude = (root / ".git/info/exclude").read_text(encoding="utf-8")
    assert f"/{CHECKOUT_PATH}/" in exclude.splitlines()
    assert git(root, "status", "--porcelain", "--", CHECKOUT_PATH) == ""


def test_refresh_fails_when_existing_checkout_is_dirty(tmp_path: Path):
    remote, _commit = make_local_pyqtgraph_repo(tmp_path)
    root = tmp_path / "workspace"
    root.mkdir()

    first_refresh = subprocess.run(
        [
            sys.executable,
            str(SCRIPT.resolve()),
            "--refresh",
            "--root",
            str(root),
            "--repo",
            str(remote),
        ],
        text=True,
        capture_output=True,
    )
    assert first_refresh.returncode == 0, first_refresh.stderr

    checkout = root / CHECKOUT_PATH
    (checkout / "README.md").write_text("dirty local edit\n", encoding="utf-8")

    dirty_refresh = subprocess.run(
        [
            sys.executable,
            str(SCRIPT.resolve()),
            "--refresh",
            "--root",
            str(root),
            "--repo",
            str(remote),
        ],
        text=True,
        capture_output=True,
    )

    assert dirty_refresh.returncode != 0
    assert "uncommitted changes" in dirty_refresh.stderr


def test_refresh_existing_checkout_fetches_requested_repo(tmp_path: Path):
    first_remote, _first_commit = make_local_pyqtgraph_repo(
        tmp_path, "first-remote", "first fixture\n"
    )
    second_remote, second_commit = make_local_pyqtgraph_repo(
        tmp_path, "second-remote", "second fixture\n"
    )
    root = tmp_path / "workspace"
    root.mkdir()

    first_refresh = subprocess.run(
        [
            sys.executable,
            str(SCRIPT.resolve()),
            "--refresh",
            "--root",
            str(root),
            "--repo",
            str(first_remote),
        ],
        text=True,
        capture_output=True,
    )
    assert first_refresh.returncode == 0, first_refresh.stderr

    second_refresh = subprocess.run(
        [
            sys.executable,
            str(SCRIPT.resolve()),
            "--refresh",
            "--root",
            str(root),
            "--repo",
            str(second_remote),
        ],
        text=True,
        capture_output=True,
    )

    assert second_refresh.returncode == 0, second_refresh.stderr
    assert git(root / CHECKOUT_PATH, "rev-parse", "HEAD") == second_commit
    lock = load_yaml(root / "reference/source.lock")
    assert lock["repo"] == str(second_remote)
    assert lock["pinned_commit"] == second_commit


def test_script_is_executable():
    assert os.access(SCRIPT, os.X_OK)


def test_p2_08_oracle_check_uses_checked_in_fixture_without_optional_checkout(tmp_path: Path):
    root = tmp_path / "workspace"
    fixture = root / "oracle" / "fixtures" / "P2_08" / "signal_proxy_timer_oracle.json"
    fixture.parent.mkdir(parents=True)
    script = Path("oracle/scripts/generate_P2_08_signal_proxy_timer_oracle.py")
    script_copy = root / script
    script_copy.parent.mkdir(parents=True)
    shutil.copy2(script, script_copy)
    fixture.write_text(
        Path("oracle/fixtures/P2_08/signal_proxy_timer_oracle.json").read_text(encoding="utf-8"),
        encoding="utf-8",
    )

    result = subprocess.run(
        [sys.executable, str(script_copy), "--check"],
        cwd=root,
        text=True,
        capture_output=True,
    )

    assert result.returncode == 0, result.stderr
    assert "P2.08 oracle fixture OK" in result.stdout


def test_p2_08_oracle_require_source_keeps_strict_source_validation(tmp_path: Path):
    root = tmp_path / "workspace"
    script = Path("oracle/scripts/generate_P2_08_signal_proxy_timer_oracle.py")
    script_copy = root / script
    script_copy.parent.mkdir(parents=True)
    shutil.copy2(script, script_copy)

    result = subprocess.run(
        [sys.executable, str(script_copy), "--check", "--require-source"],
        cwd=root,
        text=True,
        capture_output=True,
    )

    assert result.returncode != 0
    assert "Pinned PyQtGraph checkout is unavailable" in result.stderr
