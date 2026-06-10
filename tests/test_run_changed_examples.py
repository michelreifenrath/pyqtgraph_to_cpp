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


def test_manifest_registry_derives_simpleplot_from_port_manifest() -> None:
    module = load_run_changed_examples_module()

    examples = module.manifest_examples(REPO_ROOT)
    simpleplot = [example for example in examples if example.name == "SimplePlot"]

    assert len(simpleplot) == 1
    example = simpleplot[0]
    assert example.target_path == "examples/SimplePlot.cpp"
    assert example.ctest_name == "cppqtgraph.examples.SimplePlot"
    assert example.visual_required is True
    assert "pyqtgraph/examples/SimplePlot.py" in example.aliases
    assert not hasattr(module, "EXAMPLES")


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
