from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

import yaml

SCRIPT = Path("scripts/summarize_status")


def run_cli(*args: str, root: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(SCRIPT.resolve()), "--root", str(root), *args],
        text=True,
        capture_output=True,
    )


def write_manifest(root: Path) -> dict[str, object]:
    manifest = {
        "reference": {
            "repo": "fixture://pyqtgraph",
            "ref": "pyqtgraph-0.14.0",
            "pinned_commit": "abc123",
            "docs_url": "https://pyqtgraph.readthedocs.io/",
        },
        "manifest_schema": {"status_metadata": "adopted"},
        "source_files": [
            {
                "upstream_path": "pyqtgraph/PlotData.py",
                "status": "ported",
                "completion": "complete",
            },
            {
                "upstream_path": "pyqtgraph/widgets/PlotWidget.py",
                "status": "partial",
                "completion": "partial",
            },
            {
                "upstream_path": "pyqtgraph/Point.py",
                "status": "not_started",
                "completion": "missing",
            },
        ],
        "examples": [
            {
                "upstream_path": "pyqtgraph/examples/Plotting.py",
                "status": "ported",
                "completion": "complete",
            },
            {
                "upstream_path": "pyqtgraph/examples/ImageView.py",
                "status": "not_started",
                "completion": "missing",
            },
        ],
        "example_assets": [
            {
                "upstream_path": "pyqtgraph/examples/designerExample.ui",
                "status": "not_started",
                "completion": "missing",
            }
        ],
        "classes": [
            {
                "class_name": "PlotData",
                "upstream_path": "pyqtgraph/PlotData.py",
                "status": "ported",
                "completion": "complete",
            },
            {
                "class_name": "PlotWidget",
                "upstream_path": "pyqtgraph/widgets/PlotWidget.py",
                "status": "partial",
                "completion": "partial",
            },
        ],
        "excluded": {"examples": [], "tests": []},
        "summary": {
            "source_file_count": 3,
            "example_count": 2,
            "example_asset_count": 1,
            "total_example_tree_file_count": 3,
            "class_count": 2,
            "excluded_example_count": 0,
            "excluded_test_count": 0,
        },
    }
    (root / "port_manifest.yaml").write_text(
        yaml.safe_dump(manifest, sort_keys=False), encoding="utf-8"
    )
    return manifest


def test_P0_03_help_exposes_dashboard_modes(tmp_path: Path) -> None:
    result = run_cli("--help", root=tmp_path)

    assert result.returncode == 0
    assert "--check" in result.stdout
    assert "--update-dashboard" in result.stdout
    assert "--format" in result.stdout
    assert "--require-complete" in result.stdout


def test_P0_03_outputs_manifest_driven_counts_as_markdown_and_json(
    tmp_path: Path,
) -> None:
    write_manifest(tmp_path)

    markdown = run_cli(root=tmp_path)
    json_result = run_cli("--format", "json", root=tmp_path)

    assert markdown.returncode == 0, markdown.stderr
    assert "| Source files | 3 | 1 | 1 | 1 |" in markdown.stdout
    assert "| Examples | 2 | 1 | 0 | 1 |" in markdown.stdout
    assert "| Example assets | 1 | 0 | 0 | 1 |" in markdown.stdout
    assert "| Classes | 2 | 1 | 1 | 0 |" in markdown.stdout
    assert "Pinned commit: `abc123`" in markdown.stdout
    assert json_result.returncode == 0, json_result.stderr
    status = json.loads(json_result.stdout)
    assert status["counts"]["source_files"]["total"] == 3
    assert status["counts"]["examples"]["complete"] == 1
    assert status["counts"]["classes"]["partial"] == 1


def test_P0_03_update_dashboard_and_check_are_read_only_when_current(
    tmp_path: Path,
) -> None:
    write_manifest(tmp_path)
    update = run_cli("--update-dashboard", root=tmp_path)
    dashboard_path = tmp_path / "reports" / "dashboard" / "status.md"
    before = dashboard_path.read_text(encoding="utf-8")

    check = run_cli("--check", root=tmp_path)

    assert update.returncode == 0, update.stderr
    assert update.stdout == "updated reports/dashboard/status.md\n"
    assert "<!-- dashboard-pinned-commit: abc123 -->" in before
    assert check.returncode == 0, check.stderr
    assert check.stdout == "dashboard verified: reports/dashboard/status.md\n"
    assert dashboard_path.read_text(encoding="utf-8") == before


def test_P0_03_check_rejects_missing_dashboard_without_writes(tmp_path: Path) -> None:
    write_manifest(tmp_path)

    result = run_cli("--check", root=tmp_path)

    assert result.returncode != 0
    assert "reports/dashboard/status.md is missing" in result.stderr
    assert "--update-dashboard" in result.stderr
    assert not (tmp_path / "reports").exists()


def test_P0_03_check_rejects_stale_dashboard_metadata_without_writes(
    tmp_path: Path,
) -> None:
    manifest = write_manifest(tmp_path)
    assert run_cli("--update-dashboard", root=tmp_path).returncode == 0
    dashboard_path = tmp_path / "reports" / "dashboard" / "status.md"
    before = dashboard_path.read_text(encoding="utf-8")
    manifest["summary"]["source_file_count"] = 4  # type: ignore[index]
    manifest["source_files"].append(  # type: ignore[attr-defined]
        {
            "upstream_path": "pyqtgraph/New.py",
            "status": "not_started",
            "completion": "missing",
        }
    )
    (tmp_path / "port_manifest.yaml").write_text(
        yaml.safe_dump(manifest, sort_keys=False), encoding="utf-8"
    )

    result = run_cli("--check", root=tmp_path)

    assert result.returncode != 0
    assert "reports/dashboard/status.md has stale dashboard metadata" in result.stderr
    assert "dashboard-source_files-total" in result.stderr
    assert "--update-dashboard" in result.stderr
    assert dashboard_path.read_text(encoding="utf-8") == before


def test_P0_03_check_rejects_missing_dashboard_metadata_without_writes(
    tmp_path: Path,
) -> None:
    write_manifest(tmp_path)
    assert run_cli("--update-dashboard", root=tmp_path).returncode == 0
    dashboard_path = tmp_path / "reports" / "dashboard" / "status.md"
    before = dashboard_path.read_text(encoding="utf-8")
    dashboard_path.write_text(
        "\n".join(
            line
            for line in before.splitlines()
            if "dashboard-source_files-total" not in line
        )
        + "\n",
        encoding="utf-8",
    )

    result = run_cli("--check", root=tmp_path)

    assert result.returncode != 0
    assert "reports/dashboard/status.md is missing dashboard metadata" in result.stderr
    assert "dashboard-source_files-total" in result.stderr
    assert "--update-dashboard" in result.stderr


def test_P0_03_check_rejects_inconsistent_manifest_summary(tmp_path: Path) -> None:
    manifest = write_manifest(tmp_path)
    manifest["summary"]["class_count"] = 99  # type: ignore[index]
    (tmp_path / "port_manifest.yaml").write_text(
        yaml.safe_dump(manifest, sort_keys=False), encoding="utf-8"
    )

    result = run_cli(root=tmp_path)

    assert result.returncode != 0
    assert "port_manifest.yaml summary is inconsistent" in result.stderr
    assert "class_count" in result.stderr
