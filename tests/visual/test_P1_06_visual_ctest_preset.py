from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PRESETS = ROOT / "CMakePresets.json"
CMAKE = ROOT / "CMakeLists.txt"
SMOKE_FIXTURE = "tests/visual/test_P1_06_visual_smoke.py"
TEST_NAME = "P1.06.visual.smoke"


def visual_test_preset() -> dict[str, object]:
    presets = json.loads(PRESETS.read_text(encoding="utf-8"))
    for preset in presets["testPresets"]:
        if preset["name"] == "visual":
            return preset
    raise AssertionError("CMakePresets.json must define a visual test preset")


def cmake_lists_text() -> str:
    return CMAKE.read_text(encoding="utf-8")


def test_P1_06_visual_preset_filters_visual_label_and_rejects_no_tests() -> None:
    preset = visual_test_preset()

    assert preset["filter"] == {"include": {"label": "visual"}}
    assert preset["execution"] == {"noTestsAction": "error"}


def test_P1_06_visual_smoke_ctest_is_registered_with_visual_labels() -> None:
    cmake = cmake_lists_text()

    assert re.search(rf"add_test\s*\(\s*NAME\s+{re.escape(TEST_NAME)}\b", cmake)
    assert SMOKE_FIXTURE in cmake
    assert "PG_VISUAL_ARTIFACT_DIR=" in cmake
    assert re.search(
        rf"set_tests_properties\s*\(\s*{re.escape(TEST_NAME)}\s+PROPERTIES\s+LABELS\s+\"visual;P1\.06\"",
        cmake,
        re.DOTALL,
    )
