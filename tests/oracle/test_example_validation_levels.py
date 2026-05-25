from __future__ import annotations

from collections import Counter
from pathlib import Path
from typing import Any

import yaml

REPO_ROOT = Path(__file__).resolve().parents[2]
MANIFEST_PATH = REPO_ROOT / "port_manifest.yaml"
REQUIRED_RECORD_KEYS = {"upstream_path", "name", "category", "validation"}
REQUIRED_VALIDATION_KEYS = {
    "numeric",
    "visual",
    "interaction",
    "gpt_visual_review",
}
LEVEL_VALUES = {"required", "optional", "not_applicable"}
GPT_VISUAL_VALUES = {"required_for_pr", "optional", "not_applicable"}


def load_manifest() -> dict[str, Any]:
    with MANIFEST_PATH.open(encoding="utf-8") as handle:
        manifest = yaml.safe_load(handle)
    assert isinstance(manifest, dict)
    return manifest


def validation_records(manifest: dict[str, Any]) -> list[dict[str, Any]]:
    records = manifest.get("example_validation_levels")
    assert records is not None, (
        "port_manifest.yaml is missing example_validation_levels"
    )
    assert isinstance(records, list), "example_validation_levels must be a list"
    return records


def test_example_validation_levels_cover_every_manifest_example_once() -> None:
    manifest = load_manifest()
    examples = manifest["examples"]
    records = validation_records(manifest)

    example_paths = [example["upstream_path"] for example in examples]
    record_paths: list[str] = []
    for record in records:
        path = record.get("upstream_path")
        assert isinstance(path, str), record
        record_paths.append(path)

    assert len(records) == manifest["example_inventory_summary"]["example_count"]
    assert len(records) == len(examples)

    duplicate_paths = sorted(
        path for path, count in Counter(record_paths).items() if count > 1
    )
    assert not duplicate_paths, f"duplicate validation records: {duplicate_paths}"

    missing = sorted(set(example_paths) - set(record_paths))
    extra = sorted(set(record_paths) - set(example_paths))
    assert not missing, f"missing validation records: {missing}"
    assert not extra, f"validation records for unknown examples: {extra}"


def test_example_validation_level_records_match_example_identity() -> None:
    manifest = load_manifest()
    examples_by_path = {
        example["upstream_path"]: example for example in manifest["examples"]
    }

    for record in validation_records(manifest):
        assert set(record) == REQUIRED_RECORD_KEYS, record
        example = examples_by_path[record["upstream_path"]]
        assert record["name"] == example["name"]
        assert record["category"] == example["category"]

        validation = record["validation"]
        assert isinstance(validation, dict), record["upstream_path"]
        assert set(validation) == REQUIRED_VALIDATION_KEYS, record["upstream_path"]


def test_representative_example_validation_categories_are_present() -> None:
    manifest = load_manifest()
    records_by_path = {
        record["upstream_path"]: record["validation"]
        for record in validation_records(manifest)
    }

    assert records_by_path["pyqtgraph/examples/Arrow.py"] == {
        "numeric": "optional",
        "visual": "required",
        "interaction": "optional",
        "gpt_visual_review": "required_for_pr",
    }
    assert records_by_path["pyqtgraph/examples/CLIexample.py"] == {
        "numeric": "optional",
        "visual": "required",
        "interaction": "optional",
        "gpt_visual_review": "required_for_pr",
    }
    assert records_by_path["pyqtgraph/examples/ColorButton.py"] == {
        "numeric": "optional",
        "visual": "required",
        "interaction": "required",
        "gpt_visual_review": "required_for_pr",
    }
    assert records_by_path["pyqtgraph/examples/optics/pyoptic.py"] == {
        "numeric": "required",
        "visual": "not_applicable",
        "interaction": "not_applicable",
        "gpt_visual_review": "not_applicable",
    }
    assert records_by_path["pyqtgraph/examples/__init__.py"] == {
        "numeric": "not_applicable",
        "visual": "not_applicable",
        "interaction": "not_applicable",
        "gpt_visual_review": "not_applicable",
    }


def test_example_validation_level_values_and_policy_invariants() -> None:
    manifest = load_manifest()

    for record in validation_records(manifest):
        validation = record["validation"]
        path = record["upstream_path"]

        assert validation["numeric"] in LEVEL_VALUES, path
        assert validation["visual"] in LEVEL_VALUES, path
        assert validation["interaction"] in LEVEL_VALUES, path
        assert validation["gpt_visual_review"] in GPT_VISUAL_VALUES, path

        if validation["visual"] == "not_applicable":
            assert validation["gpt_visual_review"] == "not_applicable", path

        if validation["visual"] == "required":
            assert validation["gpt_visual_review"] == "required_for_pr", path

        if validation["interaction"] == "required":
            assert validation["visual"] == "required", path
