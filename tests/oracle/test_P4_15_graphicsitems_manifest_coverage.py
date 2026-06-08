from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path
from typing import Any

import pytest
import yaml

SCRIPT = Path("scripts/generate_manifest")
REPO_ROOT = Path(__file__).resolve().parents[2]
GRAPHICS_ITEMS_SUBSYSTEM = "graphicsItems"
EXPECTED_GRAPHICS_ITEMS_SOURCE_FILES = 47


def run_cli(*args: str, root: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str((REPO_ROOT / SCRIPT).resolve()), *args],
        cwd=root or REPO_ROOT,
        text=True,
        capture_output=True,
    )


def load_generated_manifest(root: Path | None = None) -> dict[str, Any]:
    result = run_cli("--format", "json", root=root)
    assert result.returncode == 0, result.stderr
    return json.loads(result.stdout)


def graphicsitems_source_file_counts(manifest: dict[str, Any]) -> dict[str, int]:
    rows = [
        row
        for row in manifest.get("source_files", [])
        if isinstance(row, dict) and row.get("subsystem") == GRAPHICS_ITEMS_SUBSYSTEM
    ]
    presence = {
        "all": sum(1 for row in rows if row.get("target_presence") == "all"),
        "some": sum(1 for row in rows if row.get("target_presence") == "some"),
        "none": sum(1 for row in rows if row.get("target_presence") == "none"),
    }
    return {
        "total": len(rows),
        "target_presence_all": presence["all"],
        "target_presence_some": presence["some"],
        "target_presence_none": presence["none"],
    }


def graphicsitems_dashboard_line(counts: dict[str, int]) -> str:
    return (
        f"{GRAPHICS_ITEMS_SUBSYSTEM}: "
        f"{counts['target_presence_all']}/{counts['total']} target files present"
    )


def strip_manifest_status_metadata(manifest: dict[str, Any]) -> dict[str, Any]:
    stripped = dict(manifest)
    for section in ("source_files", "examples", "example_assets", "classes"):
        stripped[section] = [
            {
                key: value
                for key, value in row.items()
                if key not in ("status", "completion", "target_presence", "completion_evidence")
            }
            for row in manifest.get(section, [])
            if isinstance(row, dict)
        ]
    return stripped


def test_P4_15_graphicsitems_manifest_dashboard_reports_complete_coverage() -> None:
    manifest = load_generated_manifest()
    counts = graphicsitems_source_file_counts(manifest)

    assert counts["total"] == EXPECTED_GRAPHICS_ITEMS_SOURCE_FILES
    assert counts["target_presence_all"] == EXPECTED_GRAPHICS_ITEMS_SOURCE_FILES
    assert counts["target_presence_some"] == 0
    assert counts["target_presence_none"] == 0
    assert graphicsitems_dashboard_line(counts) == "graphicsItems: 47/47 target files present"


def test_P4_15_manifest_dashboard_output_is_deterministic() -> None:
    first = run_cli("--format", "json")
    second = run_cli("--format", "json")

    assert first.returncode == 0, first.stderr
    assert second.returncode == 0, second.stderr
    assert first.stdout == second.stdout

    first_counts = graphicsitems_source_file_counts(json.loads(first.stdout))
    second_counts = graphicsitems_source_file_counts(json.loads(second.stdout))
    assert first_counts == second_counts
    assert graphicsitems_dashboard_line(first_counts) == graphicsitems_dashboard_line(
        second_counts
    )


@pytest.mark.parametrize("bad_manifest", ["missing_status", "stale_summary"])
def test_P4_15_check_mode_rejects_stale_or_inconsistent_graphicsitems_metadata(
    tmp_path: Path, bad_manifest: str
) -> None:
    root = tmp_path / "workspace"
    root.mkdir()
    generated = load_generated_manifest()
    gi_rows = [
        row
        for row in generated["source_files"]
        if row.get("subsystem") == GRAPHICS_ITEMS_SUBSYSTEM
    ]
    assert gi_rows, "fixture requires graphicsItems manifest rows"

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
            if row.get("subsystem") == GRAPHICS_ITEMS_SUBSYSTEM:
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


def write_reference_lock(root: Path, reference: dict[str, Any]) -> None:
    (root / "reference").mkdir(parents=True, exist_ok=True)
    lock = dict(reference)
    lock.setdefault("checkout_path", "reference/pyqtgraph")
    (root / "reference" / "source.lock").write_text(
        yaml.safe_dump(lock, sort_keys=False),
        encoding="utf-8",
    )


def test_P4_15_check_mode_rejects_stripped_graphicsitems_status_metadata(
    tmp_path: Path,
) -> None:
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
