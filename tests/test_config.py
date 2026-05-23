from pathlib import Path

import pytest

from automation.pi_symphony.config import ConfigError, WorkflowConfig, load_workflow


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
