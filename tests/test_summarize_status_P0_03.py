import subprocess
import sys
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "summarize_status"


def write_manifest(root: Path, *, source_count: int = 2, class_count: int = 3) -> None:
    source_rows = "\n".join(
        f"""
- upstream_path: pyqtgraph/Source{index}.py
  target_header_path: include/cppqtgraph/Source{index}.hpp
  target_source_path: src/cppqtgraph/Source{index}.cpp
  status: ported
  completion: complete
  completion_evidence:
    type: focused_test
    artifact_path: reports/issues/P0.03/source-{index}.txt""".rstrip()
        for index in range(source_count)
    )
    class_rows = "\n".join(
        f"""
- class_name: Class{index}
  upstream_path: pyqtgraph/Source{index % max(source_count, 1)}.py
  target_header_path: include/cppqtgraph/Class{index}.hpp
  target_source_path: src/cppqtgraph/Class{index}.cpp
  status: ported
  completion: complete
  completion_evidence:
    type: focused_test
    artifact_path: reports/issues/P0.03/class-{index}.txt""".rstrip()
        for index in range(class_count)
    )
    (root / "port_manifest.yaml").write_text(
        f"""
summary:
  source_file_count: {source_count}
  example_count: 1
  example_asset_count: 1
  total_example_tree_file_count: 2
  class_count: {class_count}
source_files:
{source_rows}
examples:
- upstream_path: pyqtgraph/examples/Dashboard.py
  target_source_path: examples/Dashboard.cpp
  name: Dashboard
  category: root
  status: ported
  completion: complete
  completion_evidence:
    type: focused_test
    artifact_path: reports/issues/P0.03/example.txt
example_assets:
- upstream_path: pyqtgraph/examples/Dashboard.ui
  target_path: examples/Dashboard.ui
  status: ported
  completion: complete
  completion_evidence:
    type: focused_test
    artifact_path: reports/issues/P0.03/asset.txt
classes:
{class_rows}
example_validation_levels:
- upstream_path: pyqtgraph/examples/Dashboard.py
  name: Dashboard
  category: root
  validation:
    numeric: required
    visual: not_applicable
    interaction: not_applicable
    gpt_visual_review: not_applicable
""".lstrip(),
        encoding="utf-8",
    )


def create_targets_and_evidence(root: Path, *, source_count: int = 2, class_count: int = 3) -> None:
    paths = [
        "examples/Dashboard.cpp",
        "examples/Dashboard.ui",
        "reports/issues/P0.03/example.txt",
        "reports/issues/P0.03/asset.txt",
    ]
    for index in range(source_count):
        paths.extend(
            [
                f"include/cppqtgraph/Source{index}.hpp",
                f"src/cppqtgraph/Source{index}.cpp",
                f"reports/issues/P0.03/source-{index}.txt",
            ]
        )
    for index in range(class_count):
        paths.extend(
            [
                f"include/cppqtgraph/Class{index}.hpp",
                f"src/cppqtgraph/Class{index}.cpp",
                f"reports/issues/P0.03/class-{index}.txt",
            ]
        )
    for relative_path in paths:
        path = root / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("P0.03 dashboard fixture\n", encoding="utf-8")


def run_summary(root: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(SCRIPT), *args],
        cwd=root,
        text=True,
        capture_output=True,
        check=False,
    )


def test_P0_03_dashboard_reports_manifest_counts(tmp_path: Path) -> None:
    write_manifest(tmp_path, source_count=2, class_count=3)
    create_targets_and_evidence(tmp_path, source_count=2, class_count=3)

    result = run_summary(tmp_path)

    assert result.returncode == 0, result.stderr
    assert "source_files: total=2 ported=2 complete=2 incomplete=0" in result.stdout
    assert "examples: total=1 ported=1 complete=1 incomplete=0" in result.stdout
    assert "example_assets: total=1 ported=1 complete=1 incomplete=0" in result.stdout
    assert "classes: total=3 ported=3 complete=3 incomplete=0" in result.stdout
    assert "example_validation_levels: total=1 numeric_required=1" in result.stdout


def test_P0_03_dashboard_rejects_inconsistent_summary_metadata(tmp_path: Path) -> None:
    write_manifest(tmp_path, source_count=1, class_count=1)
    manifest = (tmp_path / "port_manifest.yaml").read_text(encoding="utf-8")
    (tmp_path / "port_manifest.yaml").write_text(
        manifest.replace("source_file_count: 1", "source_file_count: 2"),
        encoding="utf-8",
    )

    result = run_summary(tmp_path)

    assert result.returncode != 0
    assert "summary.source_file_count=2 does not match source_files length 1" in result.stderr


def test_P0_03_dashboard_rejects_stale_complete_target_metadata(tmp_path: Path) -> None:
    write_manifest(tmp_path, source_count=1, class_count=1)
    create_targets_and_evidence(tmp_path, source_count=1, class_count=1)
    (tmp_path / "include/cppqtgraph/Source0.hpp").unlink()

    result = run_summary(tmp_path, "--require-complete")

    assert result.returncode != 0
    assert "require_complete: failed" in result.stderr
    assert "source_files: 1 incomplete" in result.stderr
    assert (
        "source_files[0] complete metadata points to missing target file: "
        "target_header_path=include/cppqtgraph/Source0.hpp"
    ) in result.stderr
