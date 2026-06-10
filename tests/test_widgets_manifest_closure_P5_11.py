from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path
from typing import Any

import pytest
import yaml

SCRIPT = Path("scripts/generate_manifest")
SUMMARIZE_STATUS = Path("scripts/summarize_status")
REPO_ROOT = Path(__file__).resolve().parents[1]
WIDGETS_SUBSYSTEM = "widgets"
EXPECTED_WIDGET_SOURCE_FILES = 33


def run_cli(*args: str, root: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str((REPO_ROOT / SCRIPT).resolve()), *args],
        cwd=root or REPO_ROOT,
        text=True,
        capture_output=True,
    )


def run_summarize_status(root: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str((REPO_ROOT / SUMMARIZE_STATUS).resolve())],
        cwd=root or REPO_ROOT,
        text=True,
        capture_output=True,
    )


def load_generated_manifest(root: Path | None = None) -> dict[str, Any]:
    result = run_cli("--format", "json", root=root)
    assert result.returncode == 0, result.stderr
    return json.loads(result.stdout)


def widgets_source_file_counts(manifest: dict[str, Any]) -> dict[str, int]:
    rows = [
        row
        for row in manifest.get("source_files", [])
        if isinstance(row, dict) and row.get("subsystem") == WIDGETS_SUBSYSTEM
    ]
    return {
        "total": len(rows),
        "complete": sum(1 for row in rows if row.get("completion") == "complete"),
        "ported": sum(1 for row in rows if row.get("status") == "ported"),
        "target_presence_all": sum(
            1 for row in rows if row.get("target_presence") == "all"
        ),
    }


def widgets_dashboard_line(counts: dict[str, int]) -> str:
    return f"{WIDGETS_SUBSYSTEM}: {counts['complete']}/{counts['total']} complete"


def strip_manifest_status_metadata(manifest: dict[str, Any]) -> dict[str, Any]:
    stripped = dict(manifest)
    for section in ("source_files", "examples", "example_assets", "classes"):
        stripped[section] = [
            {
                key: value
                for key, value in row.items()
                if key
                not in ("status", "completion", "target_presence", "completion_evidence")
            }
            for row in manifest.get(section, [])
            if isinstance(row, dict)
        ]
    return stripped


def write_reference_lock(root: Path, reference: dict[str, Any]) -> None:
    (root / "reference").mkdir(parents=True, exist_ok=True)
    lock = dict(reference)
    lock.setdefault("checkout_path", "reference/pyqtgraph")
    (root / "reference" / "source.lock").write_text(
        yaml.safe_dump(lock, sort_keys=False),
        encoding="utf-8",
    )


def test_P5_11_widgets_manifest_dashboard_reports_complete_coverage() -> None:
    manifest = load_generated_manifest()
    counts = widgets_source_file_counts(manifest)

    assert counts["total"] == EXPECTED_WIDGET_SOURCE_FILES
    assert counts["complete"] == EXPECTED_WIDGET_SOURCE_FILES
    assert counts["ported"] == EXPECTED_WIDGET_SOURCE_FILES
    assert counts["target_presence_all"] == EXPECTED_WIDGET_SOURCE_FILES
    assert widgets_dashboard_line(counts) == "widgets: 33/33 complete"

    dashboard = run_summarize_status()
    assert dashboard.returncode == 0, dashboard.stderr
    assert widgets_dashboard_line(counts) in dashboard.stdout

    widget_rows = [
        row
        for row in manifest["source_files"]
        if row.get("subsystem") == WIDGETS_SUBSYSTEM
    ]
    for row in widget_rows:
        assert row.get("completion") == "complete"
        evidence = row.get("completion_evidence")
        assert isinstance(evidence, dict)
        assert evidence.get("type")
        assert evidence.get("artifact_path")


def test_P5_11_manifest_dashboard_output_is_deterministic() -> None:
    first = run_cli("--format", "json")
    second = run_cli("--format", "json")

    assert first.returncode == 0, first.stderr
    assert second.returncode == 0, second.stderr
    assert first.stdout == second.stdout

    first_counts = widgets_source_file_counts(json.loads(first.stdout))
    second_counts = widgets_source_file_counts(json.loads(second.stdout))
    assert first_counts == second_counts
    assert widgets_dashboard_line(first_counts) == widgets_dashboard_line(second_counts)


@pytest.mark.parametrize("bad_manifest", ["missing_status", "stale_summary"])
def test_P5_11_check_mode_rejects_stale_or_inconsistent_widgets_metadata(
    tmp_path: Path, bad_manifest: str
) -> None:
    root = tmp_path / "workspace"
    root.mkdir()
    generated = load_generated_manifest()
    widget_rows = [
        row
        for row in generated["source_files"]
        if row.get("subsystem") == WIDGETS_SUBSYSTEM
    ]
    assert widget_rows, "fixture requires widgets manifest rows"

    manifest = {
        "reference": generated["reference"],
        "manifest_schema": generated["manifest_schema"],
        "source_files": generated["source_files"],
        "examples": generated["examples"],
        "example_assets": generated["example_assets"],
        "example_inventory_summary": generated["example_inventory_summary"],
        "classes": generated["classes"],
        "excluded": generated["excluded"],
        "summary": dict(generated["summary"]),
    }
    if bad_manifest == "missing_status":
        for row in manifest["source_files"]:
            if row.get("subsystem") == WIDGETS_SUBSYSTEM:
                row.pop("status", None)
                break
    else:
        manifest["summary"]["source_file_count"] = int(manifest["summary"]["source_file_count"]) + 1

    (root / "port_manifest.yaml").write_text(
        yaml.safe_dump(manifest, sort_keys=False),
        encoding="utf-8",
    )
    write_reference_lock(root, generated["reference"])

    result = run_cli("--root", str(root), "--check")

    assert result.returncode != 0
    assert "port_manifest.yaml" in result.stderr
    assert "--update-manifest" in result.stderr


def test_P5_11_check_mode_rejects_stripped_widgets_status_metadata(tmp_path: Path) -> None:
    root = tmp_path / "workspace"
    root.mkdir()
    generated = load_generated_manifest()
    manifest = strip_manifest_status_metadata(generated)
    manifest["manifest_schema"] = generated["manifest_schema"]
    (root / "port_manifest.yaml").write_text(
        yaml.safe_dump(manifest, sort_keys=False),
        encoding="utf-8",
    )
    write_reference_lock(root, generated["reference"])

    result = run_cli("--root", str(root), "--check")

    assert result.returncode != 0
    assert "port_manifest.yaml" in result.stderr
    assert "status" in result.stderr
