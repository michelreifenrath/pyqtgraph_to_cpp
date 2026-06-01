from __future__ import annotations

from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[2]

EXPECTED_COMMANDS = {
    "pgcpp-issue-ready.yaml": "pgcpp-issue-ready",
    "pgcpp-fix-issue.yaml": "pgcpp-fix-issue",
    "pgcpp-validate-pr.yaml": "pgcpp-validate-pr",
    "pgcpp-comprehensive-test.yaml": "pgcpp-comprehensive-test",
}


def load_workflow(name: str) -> dict[str, object]:
    workflow = ROOT / ".archon" / "workflows" / name
    assert workflow.exists(), workflow
    data = yaml.safe_load(workflow.read_text(encoding="utf-8"))
    assert isinstance(data, dict)
    return data


def test_archon_workflows_use_dag_schema() -> None:
    for workflow in (ROOT / ".archon" / "workflows").glob("*.yaml"):
        data = yaml.safe_load(workflow.read_text(encoding="utf-8"))
        assert isinstance(data.get("name"), str), workflow
        assert isinstance(data.get("description"), str), workflow
        assert "nodes" in data, workflow
        assert isinstance(data["nodes"], list), workflow
        assert data["nodes"], workflow
        for node in data["nodes"]:
            node_types = [key for key in ("command", "prompt", "bash", "script", "loop", "approval", "cancel") if key in node]
            assert len(node_types) == 1, (workflow, node)


def test_archon_command_nodes_reference_existing_commands() -> None:
    for workflow_name, command_name in EXPECTED_COMMANDS.items():
        data = load_workflow(workflow_name)
        commands = [node["command"] for node in data["nodes"] if "command" in node]
        assert command_name in commands
        assert (ROOT / ".archon" / "commands" / f"{command_name}.md").exists()


def test_archon_workflow_bash_arguments_are_quoted() -> None:
    combined = "\n".join(
        path.read_text(encoding="utf-8")
        for path in (ROOT / ".archon" / "workflows").glob("*.yaml")
    )
    assert "--issue-file \"$ARGUMENTS\"" in combined
    assert "--input \"$ARGUMENTS\"" in combined
    assert "--issue-file $ARGUMENTS" not in combined
    assert "--input $ARGUMENTS" not in combined


def test_archon_workflows_reference_existing_factory_scripts() -> None:
    scripts = {
        "scripts/factory/check_issue_ready.py",
        "scripts/factory/check_pr_scope.py",
        "scripts/factory/file_regression_issue.py",
    }
    combined = "\n".join(
        path.read_text(encoding="utf-8")
        for path in (ROOT / ".archon" / "workflows").glob("*.yaml")
    )
    for script in scripts:
        assert script in combined
        assert (ROOT / script).exists()


def test_validate_pr_command_documents_dry_run_verdict_script() -> None:
    command = (ROOT / ".archon" / "commands" / "pgcpp-validate-pr.md").read_text(encoding="utf-8")
    assert "scripts/factory/apply_pr_verdict.py" in command
    assert "dry-run by default" in command
    assert "never executes `gh`" in command
