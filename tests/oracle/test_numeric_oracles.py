from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path
from typing import Any

import pytest
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


def run_git(cwd: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["git", *args],
        cwd=cwd,
        text=True,
        capture_output=True,
        check=True,
    )


def write_reference_checkout(
    root: Path,
    point_bias: float = 0.0,
    log_offset: float = 0.0,
    checkout_path: str = CHECKOUT_PATH,
) -> str:
    checkout = root / checkout_path
    package = checkout / "pyqtgraph"
    graphics_items = package / "graphicsItems"
    graphics_items.mkdir(parents=True, exist_ok=True)

    (package / "__init__.py").write_text(
        '__version__ = "fixture-reference"\n',
        encoding="utf-8",
    )
    (package / "Point.py").write_text(
        f"""
class Point(QtCore.QPointF):
    POINT_BIAS = {point_bias!r}

    def __init__(self, *args):
        if len(args) == 1:
            value = args[0]
            if isinstance(value, Point):
                super().__init__(value.x(), value.y())
            elif isinstance(value, (int, float)):
                super().__init__(float(value), float(value))
            else:
                super().__init__(float(value[0]), float(value[1]))
        elif len(args) == 2:
            super().__init__(float(args[0]), float(args[1]))
        else:
            raise TypeError("Point requires one iterable/scalar or two coordinates")

    def __getitem__(self, index):
        if index == 0:
            return self.x()
        if index == 1:
            return self.y()
        raise IndexError(index)

    def _coerce(self, value):
        return value if isinstance(value, Point) else Point(value)

    def __mul__(self, other):
        other = self._coerce(other)
        return Point(
            self.x() * other.x() + self.POINT_BIAS,
            self.y() * other.y() + self.POINT_BIAS,
        )

    def __rmul__(self, other):
        return self.__mul__(other)

    def __add__(self, other):
        other = self._coerce(other)
        return Point(self.x() + other.x(), self.y() + other.y())

    def __radd__(self, other):
        return self.__add__(other)
""".lstrip(),
        encoding="utf-8",
    )
    (graphics_items / "__init__.py").write_text("", encoding="utf-8")
    (graphics_items / "PlotDataItem.py").write_text(
        f"""
import numpy as np


class PlotDataset:
    LOG_OFFSET = {log_offset!r}

    def __init__(self, x, y):
        self.x = np.asarray(x, dtype=float)
        self.y = np.asarray(y, dtype=float)

    def applyLogMapping(self, logMode):
        if logMode[0]:
            self.x = np.log10(self.x) + self.LOG_OFFSET
        if logMode[1]:
            self.y = np.log10(self.y) + self.LOG_OFFSET
""".lstrip(),
        encoding="utf-8",
    )

    run_git(checkout, "init", "-q")
    run_git(checkout, "add", ".")
    run_git(
        checkout,
        "-c",
        "user.name=Numeric Oracle",
        "-c",
        "user.email=oracle@example.invalid",
        "commit",
        "-q",
        "-m",
        "fake pyqtgraph reference",
    )
    run_git(checkout, "tag", REF)
    return run_git(checkout, "rev-parse", "HEAD").stdout.strip()


def write_source_lock(
    root: Path,
    repo: str = "fixture://pyqtgraph",
    commit: str | None = None,
    materialize_reference: bool = True,
    point_bias: float = 0.0,
    log_offset: float = 0.0,
) -> str:
    actual_commit = None
    if materialize_reference:
        actual_commit = write_reference_checkout(root, point_bias, log_offset)
    pinned_commit = commit if commit is not None else actual_commit or COMMIT

    (root / "reference").mkdir(parents=True, exist_ok=True)
    (root / "reference" / "source.lock").write_text(
        yaml.safe_dump(
            {
                "repo": repo,
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
    commit = write_source_lock(root)

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
        "pinned_commit": commit,
        "docs_url": DOCS_URL,
        "checkout_path": CHECKOUT_PATH,
    }
    assert manifest["summary"] == {"case_count": 3}
    assert [case["id"] for case in manifest["cases"]] == [
        "affine_transform",
        "log_mapping",
        "nan_minmax",
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
    commit = write_source_lock(root)

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
    nan_minmax_path = fixture_dir / "nan_minmax.json"
    assert affine_path.is_file()
    assert log_path.is_file()
    assert nan_minmax_path.is_file()

    affine = load_json(affine_path)
    log_mapping = load_json(log_path)
    nan_minmax = load_json(nan_minmax_path)
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
    assert affine["reference"] == {"ref": REF, "pinned_commit": commit}
    assert affine["expected"]["points"] == [[1.5, -2.0], [3.5, 4.0], [-4.5, 11.5]]
    assert affine["tolerance"] == {"absolute": 0.0, "relative": 0.0}

    assert log_mapping["schema_version"] == 1
    assert log_mapping["case"] == "log_mapping"
    assert log_mapping["expected"]["values"] == [-1.0, 0.0, 1.0, 2.0]
    assert log_mapping["tolerance"] == {"absolute": 1.0e-12, "relative": 1.0e-12}

    assert nan_minmax["schema_version"] == 1
    assert nan_minmax["case"] == "nan_minmax"
    assert nan_minmax["inputs"]["cases"][-1] == {"name": "empty", "values": []}
    expected_by_name = {case["name"]: case for case in nan_minmax["expected"]["cases"]}
    assert expected_by_name["finite"] == {
        "name": "finite",
        "nanmin": {"value": -2.5},
        "nanmax": {"value": 9.25},
    }
    assert expected_by_name["mixed_nan"] == {
        "name": "mixed_nan",
        "nanmin": {"value": -7.0},
        "nanmax": {"value": 3.0},
    }
    assert expected_by_name["infinities"] == {
        "name": "infinities",
        "nanmin": {"value": "-Infinity"},
        "nanmax": {"value": "Infinity"},
    }
    assert expected_by_name["all_nan"] == {
        "name": "all_nan",
        "nanmin": {"value": "NaN"},
        "nanmax": {"value": "NaN"},
    }
    assert expected_by_name["empty"]["nanmin"]["error"]["type"] == "ValueError"
    assert expected_by_name["empty"]["nanmax"]["error"]["type"] == "ValueError"
    assert nan_minmax["tolerance"] == {"absolute": 0.0, "relative": 0.0}


def test_expected_values_come_from_reference_runtime(tmp_path: Path) -> None:
    root = tmp_path / "workspace"
    write_source_lock(root, point_bias=10.0, log_offset=5.0)

    result = run_cli("--root", str(root), "--format", "json")

    assert result.returncode == 0, result.stderr
    fixture_dir = root / "oracle" / "fixtures" / "numeric"
    affine = load_json(fixture_dir / "affine_transform.json")
    log_mapping = load_json(fixture_dir / "log_mapping.json")
    nan_minmax = load_json(fixture_dir / "nan_minmax.json")
    assert affine["expected"]["points"] == [
        [11.5, 8.0],
        [13.5, 14.0],
        [5.5, 21.5],
    ]
    assert affine["expected"]["points"] != [[1.5, -2.0], [3.5, 4.0], [-4.5, 11.5]]
    assert log_mapping["expected"]["values"] == [4.0, 5.0, 6.0, 7.0]
    assert [case["name"] for case in nan_minmax["expected"]["cases"]] == [
        "finite",
        "mixed_nan",
        "infinities",
        "all_nan",
        "empty",
    ]


def test_materializes_missing_reference_checkout_from_lock(tmp_path: Path) -> None:
    remote_root = tmp_path / "remote"
    remote_checkout_path = "pyqtgraph-reference"
    commit = write_reference_checkout(
        remote_root,
        point_bias=2.0,
        log_offset=3.0,
        checkout_path=remote_checkout_path,
    )
    root = tmp_path / "workspace"
    write_source_lock(
        root,
        repo=str(remote_root / remote_checkout_path),
        commit=commit,
        materialize_reference=False,
    )

    result = run_cli("--root", str(root), "--check")

    assert result.returncode == 0, result.stderr
    assert "numeric oracles verified" in result.stdout
    assert not (root / CHECKOUT_PATH).exists()


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


def test_rejects_unusable_reference_checkout_source(tmp_path: Path) -> None:
    root = tmp_path / "workspace"
    write_source_lock(root, materialize_reference=False)

    result = run_cli("--root", str(root))

    assert result.returncode != 0
    assert result.stderr.startswith("error:")
    assert "pinned-source fallback failed" in result.stderr
    assert "could not materialize" in result.stderr


def test_rejects_reference_checkout_commit_mismatch(tmp_path: Path) -> None:
    root = tmp_path / "workspace"
    write_source_lock(root, commit="0" * 40)

    result = run_cli("--root", str(root))

    assert result.returncode != 0
    assert result.stderr.startswith("error:")
    assert "reference checkout commit mismatch" in result.stderr


@pytest.mark.parametrize("dirty_state", ["untracked", "modified", "deleted"])
def test_rejects_dirty_reference_checkout_at_pinned_commit(
    tmp_path: Path,
    dirty_state: str,
) -> None:
    root = tmp_path / "workspace"
    commit = write_source_lock(root)
    checkout = root / CHECKOUT_PATH
    assert run_git(checkout, "rev-parse", "HEAD").stdout.strip() == commit

    if dirty_state == "untracked":
        (checkout / "pyqtgraph" / "Bogus.py").write_text(
            "# untracked\n", encoding="utf-8"
        )
    elif dirty_state == "modified":
        (checkout / "pyqtgraph" / "Point.py").write_text(
            "# changed\n", encoding="utf-8"
        )
    else:
        (checkout / "pyqtgraph" / "Point.py").unlink()

    result = run_cli("--root", str(root))

    assert result.returncode != 0
    assert result.stderr.startswith("error:")
    assert "must be clean" in result.stderr
    assert "non-deterministic" in result.stderr


def test_check_mode_allows_empty_fixture_dir_without_writes(tmp_path: Path) -> None:
    root = tmp_path / "workspace"
    write_source_lock(root)
    fixture_dir = root / "oracle" / "fixtures" / "numeric"
    fixture_dir.mkdir(parents=True)
    (fixture_dir / ".gitkeep").write_text("", encoding="utf-8")
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")

    assert result.returncode == 0, result.stderr
    assert "numeric oracles verified" in result.stdout
    assert snapshot_tree(root) == before
    assert not list(fixture_dir.glob("*.json"))


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
