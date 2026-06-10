import subprocess
import sys
from pathlib import Path

import yaml


REPO_ROOT = Path(__file__).resolve().parents[1]
CHECK_MANIFEST_OWNERSHIP = REPO_ROOT / "scripts" / "check_manifest_ownership"


def run_check(root: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [
            sys.executable,
            str(CHECK_MANIFEST_OWNERSHIP),
            "--manifest",
            "port_manifest.yaml",
            "--ownership",
            "ownership.yaml",
        ],
        cwd=root,
        text=True,
        capture_output=True,
        check=False,
    )


def write_yaml(path: Path, data: dict) -> None:
    path.write_text(yaml.safe_dump(data, sort_keys=False), encoding="utf-8")


def write_manifest(root: Path) -> None:
    write_yaml(
        root / "port_manifest.yaml",
        {
            "source_files": [
                {
                    "upstream_path": "pyqtgraph/Foo.py",
                    "target_header_path": "include/cppqtgraph/Foo.hpp",
                    "target_source_path": "src/cppqtgraph/Foo.cpp",
                    "status": "not_started",
                    "completion": "missing",
                }
            ],
            "examples": [
                {
                    "upstream_path": "pyqtgraph/examples/Foo.py",
                    "target_source_path": "examples/Foo.cpp",
                    "name": "Foo",
                    "category": "root",
                    "status": "not_started",
                    "completion": "missing",
                }
            ],
            "example_assets": [
                {
                    "upstream_path": "pyqtgraph/examples/data/foo.npy",
                    "target_path": "examples/data/foo.npy",
                    "status": "not_started",
                    "completion": "missing",
                }
            ],
            "classes": [
                {
                    "class_name": "Foo",
                    "upstream_path": "pyqtgraph/Foo.py",
                    "target_header_path": "include/cppqtgraph/Foo.hpp",
                    "target_source_path": "src/cppqtgraph/Foo.cpp",
                    "status": "not_started",
                    "completion": "missing",
                }
            ],
            "example_validation_levels": [
                {
                    "upstream_path": "pyqtgraph/examples/Foo.py",
                    "name": "Foo",
                    "category": "root",
                    "validation": {
                        "numeric": "not_applicable",
                        "visual": "not_applicable",
                        "interaction": "not_applicable",
                        "gpt_visual_review": "not_applicable",
                    },
                }
            ],
        },
    )


def write_ownership(root: Path, *, include_classes: bool = True) -> None:
    rules = [
        {"issue": "P0.04", "section": "source_files", "patterns": ["pyqtgraph/**"]},
        {"issue": "P0.04", "section": "examples", "patterns": ["pyqtgraph/examples/**"]},
        {"issue": "P0.04", "section": "example_assets", "patterns": ["pyqtgraph/examples/**"]},
        {
            "issue": "P0.04",
            "section": "example_validation_levels",
            "patterns": ["pyqtgraph/examples/**"],
        },
    ]
    if include_classes:
        rules.append({"issue": "P0.04", "section": "classes", "patterns": ["pyqtgraph/**"]})
    write_yaml(root / "ownership.yaml", {"version": 1, "claims": [], "manifest_ownership": rules})


def test_P0_04_manifest_ownership_passes_when_every_entry_is_owned(tmp_path: Path) -> None:
    write_manifest(tmp_path)
    write_ownership(tmp_path)

    result = run_check(tmp_path)

    assert result.returncode == 0, result.stderr
    assert result.stdout == "manifest ownership check passed\n"


def test_P0_04_manifest_ownership_fails_for_unowned_entry(tmp_path: Path) -> None:
    write_manifest(tmp_path)
    write_ownership(tmp_path, include_classes=False)

    result = run_check(tmp_path)

    assert result.returncode != 0
    assert "unowned manifest entry: classes[0] pyqtgraph/Foo.py" in result.stderr


def test_P0_04_manifest_ownership_rejects_stale_owner_section(tmp_path: Path) -> None:
    write_yaml(tmp_path / "port_manifest.yaml", {"source_files": []})
    write_yaml(
        tmp_path / "ownership.yaml",
        {
            "version": 1,
            "claims": [],
            "manifest_ownership": [
                {"issue": "P0.04", "section": "classes", "patterns": ["pyqtgraph/**"]}
            ],
        },
    )

    result = run_check(tmp_path)

    assert result.returncode != 0
    assert "manifest_ownership[0].section=classes is stale: missing from manifest" in result.stderr
