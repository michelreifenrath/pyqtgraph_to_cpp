from __future__ import annotations

import subprocess
import sys
from pathlib import Path
from typing import Any

import yaml

SCRIPT = Path("scripts/check_manifest_ownership")


def run_gate(root: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(SCRIPT.resolve()), "--root", str(root)],
        text=True,
        capture_output=True,
    )


def write_yaml(path: Path, data: Any) -> None:
    path.write_text(yaml.safe_dump(data, sort_keys=False), encoding="utf-8")


def write_fixture(root: Path, *, owners: list[dict[str, Any]]) -> None:
    write_yaml(
        root / "port_manifest.yaml",
        {
            "source_files": [
                {"upstream_path": "pyqtgraph/PlotData.py"},
                {"upstream_path": "pyqtgraph/widgets/PlotWidget.py"},
            ],
            "classes": [
                {
                    "class_name": "PlotData",
                    "upstream_path": "pyqtgraph/PlotData.py",
                },
                {
                    "class_name": "PlotWidget",
                    "upstream_path": "pyqtgraph/widgets/PlotWidget.py",
                },
            ],
            "examples": [
                {"upstream_path": "pyqtgraph/examples/Example.py"},
            ],
            "example_assets": [
                {"upstream_path": "pyqtgraph/examples/designerExample.ui"},
            ],
            "example_validation_levels": [
                {"upstream_path": "pyqtgraph/examples/Example.py"},
            ],
        },
    )
    write_yaml(
        root / "ownership.yaml", {"version": 1, "claims": [], "manifest_owners": owners}
    )


def test_P0_04_manifest_ownership_passes_with_covering_selectors(
    tmp_path: Path,
) -> None:
    write_fixture(
        tmp_path,
        owners=[
            {
                "issue": "P1.01",
                "source_selectors": ["pyqtgraph/**"],
                "example_selectors": ["pyqtgraph/examples/**"],
            }
        ],
    )

    result = run_gate(tmp_path)

    assert result.returncode == 0, result.stderr
    assert "manifest ownership check passed" in result.stdout


def test_P0_04_manifest_ownership_fails_when_manifest_row_is_unowned(
    tmp_path: Path,
) -> None:
    write_fixture(
        tmp_path,
        owners=[
            {
                "issue": "P1.01",
                "source_selectors": ["pyqtgraph/PlotData.py"],
                "example_selectors": ["pyqtgraph/examples/**"],
            }
        ],
    )

    result = run_gate(tmp_path)

    assert result.returncode == 1
    assert "unowned manifest row" in result.stderr
    assert "source_files" in result.stderr
    assert "pyqtgraph/widgets/PlotWidget.py" in result.stderr


def test_P0_04_manifest_ownership_fails_when_selector_matches_no_manifest_row(
    tmp_path: Path,
) -> None:
    write_fixture(
        tmp_path,
        owners=[
            {
                "issue": "P1.01",
                "source_selectors": ["pyqtgraph/**"],
                "example_selectors": [
                    "pyqtgraph/examples/**",
                    "pyqtgraph/examples/Missing.py",
                ],
            }
        ],
    )

    result = run_gate(tmp_path)

    assert result.returncode == 1
    assert "ownership selector matched no manifest rows" in result.stderr
    assert "example_selectors" in result.stderr
    assert "pyqtgraph/examples/Missing.py" in result.stderr
