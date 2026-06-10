from __future__ import annotations

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def test_P3_13_visual_required_simpleplot_selects_visual_ctest() -> None:
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "scripts" / "run_changed_examples"),
            "SimplePlot",
            "--ctest-preset",
            "visual",
            "--dry-run",
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )

    assert result.returncode == 0, result.stderr
    assert result.stderr == ""
    assert (
        "ctest --preset visual -R '^P3\\.13\\.visual\\.SimplePlot$' --output-on-failure"
        in result.stdout.splitlines()
    )
    assert "cppqtgraph.examples.SimplePlot" not in result.stdout
