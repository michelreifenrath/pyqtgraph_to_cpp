import subprocess
import sys
from pathlib import Path


WORKFLOW = """---
tracker:
  repo: michelreifenrath/pyqtgraph_to_cpp
workspace:
  root: /home/michel/code/ai-workspaces/pyqtgraph_to_cpp
kanban:
  board_slug: pyqtgraph-to-cpp
  default_tenant: core
---
# Body
"""


def test_board_info_script_prints_board_tenant_and_tags(tmp_path: Path):
    workflow = tmp_path / "WORKFLOW.md"
    workflow.write_text(WORKFLOW, encoding="utf-8")

    result = subprocess.run(
        [
            sys.executable,
            "scripts/pi-symphony-board-info",
            "--workflow",
            str(workflow),
            "tenant:cpp",
            "tag:parser",
            "tag:build",
        ],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    )

    assert result.stdout.splitlines() == [
        "board_slug: pyqtgraph-to-cpp",
        "tenant: cpp",
        "tags: parser, build",
        "source_repo: michelreifenrath/pyqtgraph_to_cpp",
    ]


def test_cli_exposes_production_commands():
    result = subprocess.run(
        [sys.executable, "-m", "automation.pi_symphony.cli", "--help"],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    )
    assert "run-issue" in result.stdout
    assert "reconcile" in result.stdout
    assert "not implemented" not in result.stdout
