from __future__ import annotations

import json
import subprocess
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]


def run_command(*args: str, cwd: Path = ROOT) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        list(args),
        cwd=cwd,
        text=True,
        capture_output=True,
        check=False,
        timeout=120,
    )


def repository_performance_preset() -> dict[str, Any]:
    presets = json.loads((ROOT / "CMakePresets.json").read_text(encoding="utf-8"))
    for preset in presets["testPresets"]:
        if preset["name"] == "performance":
            return preset
    raise AssertionError("CMakePresets.json must define a performance test preset")


def test_P1_07_performance_preset_lists_native_smoke_test() -> None:
    configure = run_command("cmake", "--preset", "release")
    assert configure.returncode == 0, configure.stderr

    result = run_command("ctest", "--preset", "performance", "--show-only=json-v1")
    assert result.returncode == 0, result.stderr
    payload = json.loads(result.stdout)

    tests = {test["name"]: test for test in payload["tests"]}
    assert "P1.07.performance.smoke" in tests
    properties = {
        property_["name"]: property_["value"]
        for property_ in tests["P1.07.performance.smoke"].get("properties", [])
    }
    labels = set(properties.get("LABELS", []))
    assert {"performance", "P1.07"}.issubset(labels)


def test_P1_07_performance_preset_rejects_empty_label_selection(tmp_path: Path) -> None:
    performance_preset = repository_performance_preset()
    assert performance_preset.get("execution", {}).get("noTestsAction") == "error"

    (tmp_path / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.26)\n"
        "project(empty_performance_selection LANGUAGES CXX)\n"
        "include(CTest)\n"
        "if(BUILD_TESTING)\n"
        "  add_test(NAME unit.smoke COMMAND ${CMAKE_COMMAND} -E true)\n"
        "  set_tests_properties(unit.smoke PROPERTIES LABELS unit)\n"
        "endif()\n",
        encoding="utf-8",
    )
    (tmp_path / "CMakePresets.json").write_text(
        json.dumps(
            {
                "version": 6,
                "cmakeMinimumRequired": {"major": 3, "minor": 26, "patch": 0},
                "configurePresets": [
                    {
                        "name": "release",
                        "binaryDir": "${sourceDir}/build/release",
                        "cacheVariables": {"BUILD_TESTING": "ON"},
                    }
                ],
                "testPresets": [performance_preset],
            },
            indent=2,
        ),
        encoding="utf-8",
    )

    configure = run_command("cmake", "--preset", "release", cwd=tmp_path)
    assert configure.returncode == 0, configure.stderr

    result = run_command("ctest", "--preset", "performance", cwd=tmp_path)
    combined_output = result.stdout + result.stderr
    assert result.returncode != 0
    assert "No tests were found" in combined_output
