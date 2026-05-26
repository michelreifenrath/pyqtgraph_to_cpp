import subprocess
import sys
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "summarize_status"


def write_manifest(
    root: Path,
    *,
    source_status="ported",
    source_completion="complete",
    summary_source_count=1,
    include_status=True,
) -> None:
    status_lines = ""
    if include_status:
        status_lines = f"\n  status: {source_status}\n  completion: {source_completion}"
    (root / "port_manifest.yaml").write_text(
        f"""
summary:
  source_file_count: {summary_source_count}
  example_count: 1
  example_asset_count: 1
  total_example_tree_file_count: 2
  class_count: 1
source_files:
- upstream_path: pyqtgraph/Foo.py
  target_header_path: include/pyqtgraph/Foo.hpp
  target_source_path: src/pyqtgraph/Foo.cpp{status_lines}
examples:
- upstream_path: pyqtgraph/examples/Foo.py
  target_source_path: examples/Foo.cpp
  name: Foo
  category: root
  status: ported
  completion: complete
example_assets:
- upstream_path: pyqtgraph/examples/Foo.ui
  target_path: examples/Foo.ui
  status: ported
  completion: complete
classes:
- class_name: Foo
  upstream_path: pyqtgraph/Foo.py
  target_header_path: include/pyqtgraph/Foo.hpp
  target_source_path: src/pyqtgraph/Foo.cpp
  status: ported
  completion: complete
example_validation_levels:
- upstream_path: pyqtgraph/examples/Foo.py
  name: Foo
  category: root
  validation:
    numeric: optional
    visual: required
    interaction: optional
    gpt_visual_review: required_for_pr
""".lstrip(),
        encoding="utf-8",
    )


def run_summary(root: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(SCRIPT), *args],
        cwd=root,
        text=True,
        capture_output=True,
        check=False,
    )


def test_P0_09_normal_summary_output_passes(tmp_path: Path) -> None:
    write_manifest(tmp_path)

    result = run_summary(tmp_path)

    assert result.returncode == 0, result.stderr
    assert result.stderr == ""
    assert "P0.09 final acceptance status" in result.stdout
    assert "source_files: total=1 ported=1 complete=1 incomplete=0" in result.stdout
    assert "examples: total=1 ported=1 complete=1 incomplete=0" in result.stdout
    assert "summary: consistent" in result.stdout
    assert (
        "example_validation_levels: total=1 numeric_required=0 visual_required=1"
        in result.stdout
    )


def test_P0_09_require_complete_passes_on_all_complete_fixture(tmp_path: Path) -> None:
    write_manifest(tmp_path)

    result = run_summary(tmp_path, "--require-complete")

    assert result.returncode == 0, result.stderr
    assert "require_complete: satisfied" in result.stdout


def test_P0_09_inconsistent_summary_fails(tmp_path: Path) -> None:
    write_manifest(tmp_path, summary_source_count=2)

    result = run_summary(tmp_path)

    assert result.returncode != 0
    assert (
        "summary.source_file_count=2 does not match source_files length 1"
        in result.stderr
    )


def test_P0_09_missing_status_metadata_fails(tmp_path: Path) -> None:
    write_manifest(tmp_path, include_status=False)

    result = run_summary(tmp_path)

    assert result.returncode != 0
    assert "source_files[0] missing status" in result.stderr
    assert "source_files[0] missing completion" in result.stderr


def test_P0_09_invalid_status_metadata_fails(tmp_path: Path) -> None:
    write_manifest(tmp_path, source_status="stale")

    result = run_summary(tmp_path)

    assert result.returncode != 0
    assert "source_files[0] invalid status: stale" in result.stderr


def test_P0_09_validation_level_mismatch_fails(tmp_path: Path) -> None:
    write_manifest(tmp_path)
    manifest = (tmp_path / "port_manifest.yaml").read_text(encoding="utf-8")
    (tmp_path / "port_manifest.yaml").write_text(
        manifest.replace(
            "example_validation_levels:\n- upstream_path: pyqtgraph/examples/Foo.py",
            "example_validation_levels:\n- upstream_path: pyqtgraph/examples/Unknown.py",
        ),
        encoding="utf-8",
    )

    result = run_summary(tmp_path)

    assert result.returncode != 0
    assert "missing validation records: pyqtgraph/examples/Foo.py" in result.stderr
    assert (
        "validation records for unknown examples: pyqtgraph/examples/Unknown.py"
        in result.stderr
    )


def test_P0_09_require_complete_fails_with_clear_bucket(tmp_path: Path) -> None:
    write_manifest(tmp_path, source_status="not_started", source_completion="missing")

    result = run_summary(tmp_path, "--require-complete")

    assert result.returncode != 0
    assert "require_complete: failed" in result.stderr
    assert "source_files: 1 incomplete" in result.stderr
