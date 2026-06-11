from __future__ import annotations

import importlib.machinery
import importlib.util
import os
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
EXPECTED_CTEST_COMMAND = (
    "ctest --preset dev -R '^cppqtgraph\\.examples\\.SimplePlot$' --output-on-failure"
)


def load_run_changed_examples_module():
    script = REPO_ROOT / "scripts" / "run_changed_examples"
    loader = importlib.machinery.SourceFileLoader("run_changed_examples_under_test", str(script))
    spec = importlib.util.spec_from_loader(loader.name, loader)
    assert spec is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[loader.name] = module
    loader.exec_module(module)
    return module


def run_changed_examples(*args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(REPO_ROOT / "scripts/run_changed_examples"), *args],
        cwd=REPO_ROOT,
        env=os.environ.copy(),
        text=True,
        capture_output=True,
        timeout=30,
    )


def test_manifest_registry_derives_simpleplot_from_example_manifest() -> None:
    module = load_run_changed_examples_module()

    examples = module.manifest_examples(REPO_ROOT)
    simpleplot = [example for example in examples if example.name == "SimplePlot"]

    assert len(simpleplot) == 1
    example = simpleplot[0]
    assert example.target_path == "examples/SimplePlot.cpp"
    assert example.ctest_name == "cppqtgraph.examples.SimplePlot"
    assert example.visual_ctest_name == "P3.13.visual.SimplePlot"
    assert example.visual_required is True
    assert "pyqtgraph/examples/SimplePlot.py" in example.aliases
    assert not hasattr(module, "EXAMPLES")


def test_manifest_registry_derives_imageitem_ctest_metadata() -> None:
    module = load_run_changed_examples_module()

    examples = module.manifest_examples(REPO_ROOT)
    imageitem = [example for example in examples if example.name == "ImageItem"]

    assert len(imageitem) == 1
    example = imageitem[0]
    assert example.target_path == "examples/ImageItem.cpp"
    assert example.ctest_name == "cppqtgraph.examples.ImageItem"
    assert example.visual_ctest_name is None
    assert example.visual_required is True
    assert "pyqtgraph/examples/ImageItem.py" in example.aliases


def test_manifest_aliases_select_imageitem_without_duplicate_commands() -> None:
    result = run_changed_examples(
        "ImageItem",
        "pyqtgraph/examples/ImageItem.py",
        "examples/ImageItem.cpp",
        "--dry-run",
    )

    assert result.returncode == 0, result.stderr
    assert result.stdout.splitlines() == [
        "ctest --preset dev -R '^cppqtgraph\\.examples\\.ImageItem$' --output-on-failure"
    ]


def test_visual_preset_falls_back_to_normal_when_manifest_has_no_visual_ctest() -> None:
    result = run_changed_examples(
        "ImageItem",
        "--ctest-preset",
        "visual",
        "--dry-run",
    )

    assert result.returncode == 0, result.stderr
    assert result.stdout.splitlines() == [
        "ctest --preset visual -R '^cppqtgraph\\.examples\\.ImageItem$' --output-on-failure"
    ]


def test_fixture_manifest_future_visual_ctest_name(tmp_path: Path) -> None:
    module = load_run_changed_examples_module()
    examples_dir = tmp_path / "examples"
    examples_dir.mkdir()
    (examples_dir / "FuturePlot.cpp").write_text("int main() { return 0; }\n", encoding="utf-8")
    (examples_dir / "example_manifest.yaml").write_text(
        "\n".join(
            [
                "examples:",
                "  - name: FuturePlot",
                "    cpp_file: examples/FuturePlot.cpp",
                "    status: ported",
                "    validation:",
                "      visual: required",
                "    ctest:",
                "      normal: cppqtgraph.examples.FuturePlot",
                "      visual: P9.99.visual.FuturePlot",
            ]
        ),
        encoding="utf-8",
    )

    examples = module.manifest_examples(tmp_path)
    assert len(examples) == 1
    example = examples[0]
    assert example.name == "FuturePlot"
    assert example.ctest_name == "cppqtgraph.examples.FuturePlot"
    assert example.visual_ctest_name == "P9.99.visual.FuturePlot"

    visual_command = module.runner_command(example, "ctest", "visual")
    assert visual_command == [
        "ctest",
        "--preset",
        "visual",
        "-R",
        "^P9\\.99\\.visual\\.FuturePlot$",
        "--output-on-failure",
    ]


def test_dry_run_emits_manifest_ctest_commands_for_multiple_examples() -> None:
    result = run_changed_examples("SimplePlot", "ImageItem", "--dry-run")

    assert result.returncode == 0, result.stderr
    assert result.stdout.splitlines() == [
        EXPECTED_CTEST_COMMAND,
        "ctest --preset dev -R '^cppqtgraph\\.examples\\.ImageItem$' --output-on-failure",
    ]


def test_manifest_aliases_select_simpleplot_without_duplicate_commands() -> None:
    result = run_changed_examples(
        "SimplePlot",
        "pyqtgraph/examples/SimplePlot.py",
        "examples/SimplePlot.cpp",
        "--dry-run",
    )

    assert result.returncode == 0, result.stderr
    assert result.stdout.splitlines() == [EXPECTED_CTEST_COMMAND]


def test_unknown_manifest_example_selection_fails_closed_without_git() -> None:
    result = run_changed_examples("NotAnExample", "--dry-run")

    assert result.returncode == 2
    assert "unknown example selection: NotAnExample" in result.stderr
    assert "Traceback" not in result.stderr


def test_planned_manifest_example_selection_fails_closed_without_git() -> None:
    result = run_changed_examples("CLIexample", "--dry-run")

    assert result.returncode == 2
    assert "unknown example selection: CLIexample" in result.stderr
    assert "Traceback" not in result.stderr
