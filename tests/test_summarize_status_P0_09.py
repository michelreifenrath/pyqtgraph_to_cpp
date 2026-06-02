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
    include_completion_evidence=True,
) -> None:
    def evidence_lines(path: str) -> str:
        if not include_completion_evidence:
            return ""
        return f"""
  completion_evidence:
    type: focused_test
    artifact_path: {path}"""

    source_evidence = ""
    if include_status and source_completion == "complete":
        source_evidence = evidence_lines("tests/evidence/source.txt")
    status_lines = ""
    if include_status:
        status_lines = (
            f"\n  status: {source_status}\n  completion: {source_completion}"
            f"{source_evidence}"
        )
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
  completion: complete{evidence_lines("tests/evidence/example.txt")}
example_assets:
- upstream_path: pyqtgraph/examples/Foo.ui
  target_path: examples/Foo.ui
  status: ported
  completion: complete{evidence_lines("tests/evidence/asset.txt")}
classes:
- class_name: Foo
  upstream_path: pyqtgraph/Foo.py
  target_header_path: include/pyqtgraph/Foo.hpp
  target_source_path: src/pyqtgraph/Foo.cpp
  status: ported
  completion: complete{evidence_lines("tests/evidence/class.txt")}
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
    create_complete_target_files(tmp_path)

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


def test_P11_02_existing_targets_without_evidence_are_not_complete(
    tmp_path: Path,
) -> None:
    write_manifest(tmp_path, include_completion_evidence=False)
    create_complete_target_files(tmp_path)

    result = run_summary(tmp_path)

    assert result.returncode == 0, result.stderr
    assert "source_files: total=1 ported=1 complete=0 incomplete=1" in result.stdout
    assert "examples: total=1 ported=1 complete=0 incomplete=1" in result.stdout
    assert "classes: total=1 ported=1 complete=0 incomplete=1" in result.stdout


def test_P11_02_final_example_proofs_ignore_incomplete_examples(
    tmp_path: Path,
) -> None:
    write_manifest(tmp_path, include_completion_evidence=False)
    create_complete_target_files(tmp_path)
    create_final_acceptance_evidence(tmp_path)

    result = run_summary(tmp_path)

    assert result.returncode == 0, result.stderr
    assert "examples: total=1 ported=1 complete=0 incomplete=1" in result.stdout
    assert "final_acceptance_evidence: criteria=8 passed=7 example_proofs=0/0" in result.stdout


def create_complete_target_files(root: Path) -> None:
    for relative_path in (
        "include/pyqtgraph/Foo.hpp",
        "src/pyqtgraph/Foo.cpp",
        "examples/Foo.cpp",
        "examples/Foo.ui",
        "tests/evidence/source.txt",
        "tests/evidence/example.txt",
        "tests/evidence/asset.txt",
        "tests/evidence/class.txt",
    ):
        path = root / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("// P0.09 fixture\n", encoding="utf-8")


def replace_manifest_once(root: Path, old: str, new: str) -> None:
    manifest_path = root / "port_manifest.yaml"
    manifest = manifest_path.read_text(encoding="utf-8")
    manifest_path.write_text(manifest.replace(old, new, 1), encoding="utf-8")


def create_final_acceptance_evidence(
    root: Path,
    *,
    omit_criterion: str | None = None,
    failing_criterion: str | None = None,
    human_approved: bool = True,
    example_validation_runs_body: str | None = None,
) -> None:
    artifact_paths = (
        "reports/examples/Foo/validation.json",
        "reports/issues/P0.09/core-hierarchy.txt",
        "reports/issues/P0.09/platform-tests.txt",
        "build/release/reports/performance/P0.08/metrics.json",
        ".factory/logs/gates/autoreview-summary.json",
        "reports/issues/P0.09/package-install.txt",
        "reports/issues/P0.09/downstream-find-package.txt",
        "reports/issues/P0.09/human-approval.md",
    )
    for relative_path in artifact_paths:
        path = root / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("P0.09 final proof fixture\n", encoding="utf-8")

    def status(name: str) -> str:
        return "failed" if name == failing_criterion else "passed"

    criteria = {
        "example_validation_runs": f"""
    status: {status("example_validation_runs")}
    command: scripts/run_all_examples --visual --interaction --performance
    artifact_paths:
    - reports/examples/Foo/validation.json
    examples:
    - upstream_path: pyqtgraph/examples/Foo.py
      status: passed
      artifact_paths:
      - reports/examples/Foo/validation.json
""",
        "core_hierarchy_checks": f"""
    status: {status("core_hierarchy_checks")}
    command: python3 -m pytest -q tests/hierarchy
    artifact_paths:
    - reports/issues/P0.09/core-hierarchy.txt
""",
        "required_platform_tests": f"""
    status: {status("required_platform_tests")}
    command: scripts/gate merge
    artifact_paths:
    - reports/issues/P0.09/platform-tests.txt
""",
        "performance_benchmarks": f"""
    status: {status("performance_benchmarks")}
    command: scripts/run_all_examples --performance
    artifact_paths:
    - build/release/reports/performance/P0.08/metrics.json
""",
        "autoreview_status": f"""
    status: {status("autoreview_status")}
    command: scripts/run_autoreview --mode merge --base origin/main
    artifact_paths:
    - .factory/logs/gates/autoreview-summary.json
""",
        "package_install_proof": f"""
    status: {status("package_install_proof")}
    command: cmake --build --preset release --target install
    artifact_paths:
    - reports/issues/P0.09/package-install.txt
""",
        "downstream_find_package_proof": f"""
    status: {status("downstream_find_package_proof")}
    command: cmake -S reports/issues/P0.09/consumer -B build/consumer-P0_09
    artifact_paths:
    - reports/issues/P0.09/downstream-find-package.txt
""",
        "human_approval": f"""
    approved: {str(human_approved).lower()}
    reviewer: michel
    artifact_paths:
    - reports/issues/P0.09/human-approval.md
""",
    }
    if example_validation_runs_body is not None:
        criteria["example_validation_runs"] = example_validation_runs_body

    evidence = "criteria:\n"
    for name, body in criteria.items():
        if name == omit_criterion:
            continue
        evidence += f"  {name}:{body}"
    path = root / "reports/issues/P0.09/final_acceptance_evidence.yaml"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(evidence, encoding="utf-8")


def test_P0_09_require_complete_passes_on_all_complete_fixture(tmp_path: Path) -> None:
    write_manifest(tmp_path)
    create_complete_target_files(tmp_path)
    create_final_acceptance_evidence(tmp_path)

    result = run_summary(tmp_path, "--require-complete")

    assert result.returncode == 0, result.stderr
    assert (
        "final_acceptance_evidence: criteria=8 passed=8 example_proofs=1/1"
        in result.stdout
    )
    assert "require_complete: satisfied" in result.stdout


def test_P0_09_require_complete_fails_without_final_evidence_proof(
    tmp_path: Path,
) -> None:
    write_manifest(tmp_path)
    create_complete_target_files(tmp_path)

    result = run_summary(tmp_path, "--require-complete")

    assert result.returncode != 0
    assert "require_complete: failed" in result.stderr
    assert (
        "final acceptance evidence missing: "
        "reports/issues/P0.09/final_acceptance_evidence.yaml" in result.stderr
    )


def test_P0_09_require_complete_fails_without_final_criterion_proof(
    tmp_path: Path,
) -> None:
    write_manifest(tmp_path)
    create_complete_target_files(tmp_path)
    create_final_acceptance_evidence(tmp_path, omit_criterion="performance_benchmarks")

    result = run_summary(tmp_path, "--require-complete")

    assert result.returncode != 0
    assert "require_complete: failed" in result.stderr
    assert (
        "final acceptance evidence missing required criterion: performance_benchmarks"
        in result.stderr
    )


def test_P0_09_require_complete_reports_non_mapping_example_validation_runs(
    tmp_path: Path,
) -> None:
    write_manifest(tmp_path)
    create_complete_target_files(tmp_path)
    create_final_acceptance_evidence(
        tmp_path,
        example_validation_runs_body="\n  - not-a-mapping\n",
    )

    result = run_summary(tmp_path, "--require-complete")

    assert result.returncode != 0
    assert "Traceback" not in result.stderr
    assert "require_complete: failed" in result.stderr
    assert (
        "final acceptance evidence example_validation_runs must be a mapping"
        in result.stderr
    )


def test_P0_09_require_complete_fails_on_blocking_autoreview_evidence(
    tmp_path: Path,
) -> None:
    write_manifest(tmp_path)
    create_complete_target_files(tmp_path)
    create_final_acceptance_evidence(tmp_path, failing_criterion="autoreview_status")

    result = run_summary(tmp_path, "--require-complete")

    assert result.returncode != 0
    assert "require_complete: failed" in result.stderr
    assert (
        "final acceptance evidence autoreview_status status must be passed"
        in result.stderr
    )


def test_P0_09_require_complete_fails_without_human_approval(
    tmp_path: Path,
) -> None:
    write_manifest(tmp_path)
    create_complete_target_files(tmp_path)
    create_final_acceptance_evidence(tmp_path, human_approved=False)

    result = run_summary(tmp_path, "--require-complete")

    assert result.returncode != 0
    assert "require_complete: failed" in result.stderr
    assert (
        "final acceptance evidence human_approval approved must be true"
        in result.stderr
    )


def test_P0_09_require_complete_fails_on_stale_complete_metadata(
    tmp_path: Path,
) -> None:
    write_manifest(tmp_path)

    result = run_summary(tmp_path, "--require-complete")

    assert result.returncode != 0
    assert "source_files: total=1 ported=0 complete=0 incomplete=1" in result.stdout
    assert "require_complete: failed" in result.stderr
    assert "source_files: 1 incomplete" in result.stderr
    assert (
        "source_files[0] complete metadata points to missing target file: "
        "target_header_path=include/pyqtgraph/Foo.hpp" in result.stderr
    )


def test_P0_09_require_complete_rejects_absolute_target_path(tmp_path: Path) -> None:
    write_manifest(tmp_path)
    create_complete_target_files(tmp_path)
    outside_target = tmp_path.parent / f"{tmp_path.name}-outside.hpp"
    outside_target.write_text("// outside repository fixture\n", encoding="utf-8")
    replace_manifest_once(
        tmp_path,
        "target_header_path: include/pyqtgraph/Foo.hpp",
        f"target_header_path: {outside_target}",
    )

    result = run_summary(tmp_path, "--require-complete")

    assert result.returncode != 0
    assert "source_files: total=1 ported=0 complete=0 incomplete=1" in result.stdout
    assert "require_complete: failed" in result.stderr
    assert "source_files: 1 incomplete" in result.stderr
    assert (
        "source_files[0] complete metadata target path must be relative: "
        f"target_header_path={outside_target}" in result.stderr
    )


def test_P0_09_require_complete_rejects_parent_traversal_target_path(
    tmp_path: Path,
) -> None:
    write_manifest(tmp_path)
    create_complete_target_files(tmp_path)
    outside_dir = tmp_path.parent / f"{tmp_path.name}-outside"
    outside_target = outside_dir / "Foo.hpp"
    outside_dir.mkdir()
    outside_target.write_text("// outside repository fixture\n", encoding="utf-8")
    traversal_path = f"../{outside_dir.name}/Foo.hpp"
    replace_manifest_once(
        tmp_path,
        "target_source_path: src/pyqtgraph/Foo.cpp",
        f"target_source_path: {traversal_path}",
    )

    result = run_summary(tmp_path, "--require-complete")

    assert result.returncode != 0
    assert "source_files: total=1 ported=0 complete=0 incomplete=1" in result.stdout
    assert "require_complete: failed" in result.stderr
    assert "source_files: 1 incomplete" in result.stderr
    assert (
        "source_files[0] complete metadata target path escapes repository root: "
        f"target_source_path={traversal_path}" in result.stderr
    )


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
