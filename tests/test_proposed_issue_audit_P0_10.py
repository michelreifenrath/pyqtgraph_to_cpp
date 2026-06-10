import fnmatch
import subprocess
import sys
from pathlib import Path

import yaml


REPO_ROOT = Path(__file__).resolve().parents[1]
CHECK_MANIFEST_OWNERSHIP = REPO_ROOT / "scripts" / "check_manifest_ownership"


def proposed_issue_for(section: str, upstream_path: str) -> str:
    manifest = yaml.safe_load((REPO_ROOT / "port_manifest.yaml").read_text(encoding="utf-8"))
    matches = []
    for rule in manifest["proposed_issue_ownership"]["rules"]:
        if rule["section"] != section:
            continue
        if any(fnmatch.fnmatchcase(upstream_path, pattern) for pattern in rule["patterns"]):
            matches.append(rule["issue"])
    assert len(set(matches)) == 1, matches
    return matches[0]


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


def base_manifest() -> dict:
    return {
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
    }


def base_proposed_issue_ownership() -> dict:
    return {
        "version": 1,
        "rules": [
            {
                "issue": "P0.10",
                "section": "source_files",
                "path_key": "upstream_path",
                "patterns": ["pyqtgraph/*.py"],
            },
            {
                "issue": "P0.10",
                "section": "examples",
                "path_key": "upstream_path",
                "patterns": ["pyqtgraph/examples/**"],
            },
            {
                "issue": "P9.01",
                "section": "example_assets",
                "path_key": "upstream_path",
                "patterns": ["pyqtgraph/examples/**"],
            },
            {
                "issue": "P0.10",
                "section": "classes",
                "path_key": "upstream_path",
                "patterns": ["pyqtgraph/*.py"],
            },
        ],
    }


def write_ownership(root: Path, *, include_classes: bool = True) -> None:
    rules = [
        {"issue": "P0.04", "section": "source_files", "patterns": ["pyqtgraph/**"]},
        {"issue": "P0.04", "section": "examples", "patterns": ["pyqtgraph/examples/**"]},
        {"issue": "P0.04", "section": "example_assets", "patterns": ["pyqtgraph/examples/**"]},
    ]
    if include_classes:
        rules.append({"issue": "P0.04", "section": "classes", "patterns": ["pyqtgraph/**"]})
    write_yaml(root / "ownership.yaml", {"version": 1, "claims": [], "manifest_ownership": rules})


def write_fixture(
    root: Path,
    *,
    proposed_issue_ownership: dict | None = None,
    include_classes: bool = True,
) -> None:
    manifest = base_manifest()
    if not include_classes:
        manifest.pop("classes")
    if proposed_issue_ownership is not None:
        manifest["proposed_issue_ownership"] = proposed_issue_ownership
    write_yaml(root / "port_manifest.yaml", manifest)
    write_ownership(root, include_classes=include_classes)


def test_P0_10_proposed_issue_audit_passes_when_every_entry_is_mapped(
    tmp_path: Path,
) -> None:
    write_fixture(tmp_path, proposed_issue_ownership=base_proposed_issue_ownership())

    result = run_check(tmp_path)

    assert result.returncode == 0, result.stderr
    assert "manifest ownership check passed" in result.stdout
    assert "proposed issue audit passed" in result.stdout
    assert "source_files: entries=1 issues=1" in result.stdout


def test_P0_10_proposed_issue_audit_fails_for_unmapped_entry(tmp_path: Path) -> None:
    ownership = base_proposed_issue_ownership()
    ownership["rules"] = [
        rule for rule in ownership["rules"] if rule["section"] != "classes"
    ]
    write_fixture(tmp_path, proposed_issue_ownership=ownership)

    result = run_check(tmp_path)

    assert result.returncode != 0
    assert "unmapped manifest entry: classes[0] pyqtgraph/Foo.py" in result.stderr


def test_P0_10_proposed_issue_audit_rejects_stale_rule_section(tmp_path: Path) -> None:
    write_fixture(
        tmp_path,
        proposed_issue_ownership={
            "version": 1,
            "rules": [
                {
                    "issue": "P0.10",
                    "section": "classes",
                    "path_key": "upstream_path",
                    "patterns": ["pyqtgraph/**"],
                }
            ],
        },
        include_classes=False,
    )

    result = run_check(tmp_path)

    assert result.returncode != 0
    assert (
        "proposed_issue_ownership.rules[0].section=classes is stale: missing from manifest"
        in result.stderr
    )


def test_P0_10_proposed_issue_audit_rejects_inconsistent_overlap(
    tmp_path: Path,
) -> None:
    ownership = base_proposed_issue_ownership()
    ownership["rules"].insert(
        0,
        {
            "issue": "P9.99",
            "section": "source_files",
            "path_key": "upstream_path",
            "patterns": ["pyqtgraph/*.py"],
        },
    )
    write_fixture(tmp_path, proposed_issue_ownership=ownership)

    result = run_check(tmp_path)

    assert result.returncode != 0
    assert "inconsistent proposed issue mapping" in result.stderr
    assert "P0.10" in result.stderr
    assert "P9.99" in result.stderr


def test_P0_10_proposed_issue_audit_rejects_stale_pattern(tmp_path: Path) -> None:
    ownership = base_proposed_issue_ownership()
    ownership["rules"].append(
        {
            "issue": "P9.99",
            "section": "examples",
            "path_key": "upstream_path",
            "patterns": ["pyqtgraph/examples/missing/**"],
        }
    )
    write_fixture(tmp_path, proposed_issue_ownership=ownership)

    result = run_check(tmp_path)

    assert result.returncode != 0
    assert "stale proposed issue rule: examples patterns match no manifest entries" in result.stderr


def test_P0_10_real_manifest_maps_scatterplotitem_to_specific_p4_shard() -> None:
    assert proposed_issue_for("source_files", "pyqtgraph/graphicsItems/ScatterPlotItem.py") == "P4.01"
    assert proposed_issue_for("classes", "pyqtgraph/graphicsItems/ScatterPlotItem.py") == "P4.01"


def test_P0_10_real_manifest_maps_spinbox_to_specific_p5_shard() -> None:
    assert proposed_issue_for("source_files", "pyqtgraph/widgets/SpinBox.py") == "P5.19"
    assert proposed_issue_for("classes", "pyqtgraph/widgets/SpinBox.py") == "P5.19"
