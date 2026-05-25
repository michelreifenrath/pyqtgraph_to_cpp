from pathlib import Path

import pytest

from automation.pi_symphony.config import ConfigError, WorkflowConfig, load_workflow, validate_workflow_contract


VALID_WORKFLOW = """---
tracker:
  repo: michelreifenrath/pyqtgraph_to_cpp
workspace:
  root: /home/michel/code/ai-workspaces/pyqtgraph_to_cpp
pi:
  provider: openai-codex
  model: gpt-5.5
  default_thinking: xhigh
  implementation_thinking: xhigh
  use_subagents: true
policy:
  auto_merge: false
  shared_integration_files:
    - CMakeLists.txt
    - oracle/scripts/generate_numeric_oracles.py
  generated_diff_exceptions:
    - path: port_manifest.yaml
      verify_command: "python3 oracle/scripts/generate_class_inventory.py --check"
autoreview:
  enabled: true
  engine: codex
  mode: branch
  advisory: true
  mandatory_gate: true
kanban:
  board_slug: pyqtgraph-to-cpp
  board_scope: project
  tenant_strategy: tags
  default_tenant: cpp
  tenant_label_prefix: 'tenant:'
  tag_label_prefix: 'tag:'
github_output:
  style: compact
  pr_body_max_chars: 900
  comment_max_chars: 300
  comments:
    claim: false
    rework_scheduled: true
    blocked: true
    pr_ready: true
  pr_body:
    include_changed_files: false
    include_safety_section: false
    include_validation: true
  issue_body:
    template: compact
    include_agent_instructions: false
    include_workflow_rules: false
validation:
  commands:
    - "python3 -m pytest -q"
---
# Workflow body
"""


def test_config_parser_reads_kanban_settings_from_workflow(tmp_path: Path):
    workflow_path = tmp_path / "WORKFLOW.md"
    workflow_path.write_text(VALID_WORKFLOW, encoding="utf-8")

    config = load_workflow(workflow_path)

    assert config.tracker.repo == "michelreifenrath/pyqtgraph_to_cpp"
    assert config.workspace.root == "/home/michel/code/ai-workspaces/pyqtgraph_to_cpp"
    assert config.workspace.strategy == "git-worktree"
    assert config.pi.provider == "openai-codex"
    assert config.pi.model == "gpt-5.5"
    assert config.pi.default_thinking == "xhigh"
    assert config.pi.implementation_thinking == "xhigh"
    assert config.pi.use_subagents is True
    assert config.github.ready_label == "ai:ready"
    assert config.github.rework_label == "ai:rework"
    assert config.github.merge_ready_label == "ai:merge-ready"
    assert config.policy.auto_merge is False
    assert config.policy.never_push_to_main is True
    assert config.policy.shared_integration_files == ["CMakeLists.txt", "oracle/scripts/generate_numeric_oracles.py"]
    assert len(config.policy.generated_diff_exceptions) == 1
    assert config.policy.generated_diff_exceptions[0].path == "port_manifest.yaml"
    assert config.policy.generated_diff_exceptions[0].verify_command == "python3 oracle/scripts/generate_class_inventory.py --check"
    assert config.autoreview.enabled is True
    assert config.autoreview.advisory is True
    assert config.autoreview.mandatory_gate is True
    assert config.kanban.board_slug == "pyqtgraph-to-cpp"
    assert config.kanban.board_scope == "project"
    assert config.kanban.tenant_strategy == "tags"
    assert config.kanban.default_tenant == "cpp"
    assert config.kanban.tenant_label_prefix == "tenant:"
    assert config.kanban.tag_label_prefix == "tag:"
    assert config.github_output.style == "compact"
    assert config.github_output.pr_body_max_chars == 900
    assert config.github_output.comment_max_chars == 300
    assert config.github_output.comments.claim is False
    assert config.github_output.comments.rework_scheduled is True
    assert config.github_output.comments.blocked is True
    assert config.github_output.comments.pr_ready is True
    assert config.github_output.pr_body.include_changed_files is False
    assert config.github_output.pr_body.include_safety_section is False
    assert config.github_output.pr_body.include_validation is True
    assert config.github_output.issue_body.template == "compact"
    assert config.github_output.issue_body.include_agent_instructions is False
    assert config.github_output.issue_body.include_workflow_rules is False
    assert config.validation.commands == ["python3 -m pytest -q"]
    assert config.body == "# Workflow body\n"


def _contract_workflow_text() -> str:
    return """---
tracker:
  repo: owner/repo
workspace:
  root: /tmp/workspaces
pi:
  use_subagents: auto
  resources_dir: .pi
  resources_required: false
prompts:
  implement: prompts/implement-ticket.md
  rework: prompts/fix-review-findings.md
  review_context: prompts/review-context.md
policy:
  auto_merge: false
autoreview:
  enabled: true
  advisory: true
  mandatory_gate: true
---
# Lean workflow body
"""


def _write_contract_files(root: Path, *, unsafe_prompt: str | None = None) -> Path:
    workflow_path = root / "WORKFLOW.md"
    workflow_path.write_text(_contract_workflow_text(), encoding="utf-8")
    (root / "AGENTS.md").write_text("Follow the repo workflow.\n", encoding="utf-8")
    prompt_dir = root / "prompts"
    prompt_dir.mkdir()
    safe_prompt = "Do not commit, push, merge, or create PRs. Work in the assigned worktree only."
    for name in ("implement-ticket.md", "fix-review-findings.md", "review-context.md"):
        (prompt_dir / name).write_text(unsafe_prompt if unsafe_prompt is not None else safe_prompt, encoding="utf-8")
    return workflow_path


def test_config_parser_reads_prompt_and_optional_pi_resource_settings(tmp_path: Path):
    workflow_path = _write_contract_files(tmp_path)

    config = load_workflow(workflow_path)

    assert config.pi.use_subagents == "auto"
    assert config.pi.resources_dir == ".pi"
    assert config.pi.resources_required is False
    assert config.prompts.implement == "prompts/implement-ticket.md"
    assert config.prompts.rework == "prompts/fix-review-findings.md"
    assert config.prompts.review_context == "prompts/review-context.md"


def test_workflow_contract_accepts_lean_prompt_file_contract(tmp_path: Path):
    workflow_path = _write_contract_files(tmp_path)

    assert validate_workflow_contract(workflow_path) == []


def test_workflow_contract_reports_missing_required_files(tmp_path: Path):
    workflow_path = _write_contract_files(tmp_path)
    (tmp_path / "prompts" / "implement-ticket.md").unlink()

    errors = validate_workflow_contract(workflow_path)

    assert any("prompts.implement" in error and "does not exist" in error for error in errors)


def test_workflow_contract_requires_prompt_authority_boundaries(tmp_path: Path):
    workflow_path = _write_contract_files(tmp_path, unsafe_prompt="You may commit and open a PR when done.")

    errors = validate_workflow_contract(workflow_path)

    assert any("must prohibit commit/push/merge/PR creation" in error for error in errors)


def test_workflow_defaults_are_production_safe():
    config = WorkflowConfig.from_mapping(
        {
            "tracker": {"repo": "michelreifenrath/pyqtgraph_to_cpp"},
            "workspace": {"root": "/home/michel/code/ai-workspaces/pyqtgraph_to_cpp"},
        },
        body="body",
    )

    assert config.pi.use_subagents is True
    assert config.policy.auto_merge is False
    assert config.policy.never_push_to_main is True
    assert config.autoreview.enabled is True
    assert config.autoreview.advisory is True
    assert config.autoreview.mandatory_gate is True
    assert config.kanban.board_slug == "pyqtgraph-to-cpp"
    assert config.kanban.board_scope == "project"
    assert config.kanban.tenant_strategy == "tags"
    assert config.kanban.default_tenant == "cpp"
    assert config.github_output.style == "compact"
    assert config.github_output.comments.claim is False
    assert config.github_output.pr_body.include_changed_files is False
    assert config.github_output.pr_body.include_safety_section is False
    assert config.github_output.pr_body.include_validation is True
    assert config.github_output.comment_max_chars == 300


@pytest.mark.parametrize(
    "mapping, message",
    [
        ({"workspace": {"root": "/tmp/ws"}}, "tracker.repo"),
        ({"tracker": {"repo": "owner/repo"}}, "workspace.root"),
        ({"tracker": {"repo": "owner/repo"}, "workspace": {"root": "/tmp/ws"}, "pi": {"use_subagents": False}}, "pi.use_subagents"),
        ({"tracker": {"repo": "owner/repo"}, "workspace": {"root": "/tmp/ws"}, "pi": {"implementation_thinking": "maximum"}}, "pi.implementation_thinking"),
        ({"tracker": {"repo": "owner/repo"}, "workspace": {"root": "/tmp/ws"}, "policy": {"auto_merge": True}}, "policy.auto_merge"),
        ({"tracker": {"repo": "owner/repo"}, "workspace": {"root": "/tmp/ws"}, "policy": {"never_push_to_main": False}}, "policy.never_push_to_main"),
    ],
)
def test_required_policy_fields_are_validated(mapping, message):
    with pytest.raises(ConfigError, match=message):
        WorkflowConfig.from_mapping(mapping, body="")
