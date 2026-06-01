from __future__ import annotations

from dataclasses import dataclass, field, fields
from pathlib import Path
from typing import Any

import yaml


class ConfigError(ValueError):
    """Raised when WORKFLOW.md configuration is missing or invalid."""


@dataclass(frozen=True)
class TrackerConfig:
    repo: str
    kind: str = "github"
    issue_query: str = ""


@dataclass(frozen=True)
class WorkspaceConfig:
    root: str
    strategy: str = "git-worktree"
    base_branch: str = "main"
    preserve_on_success: bool = True
    cleanup_terminal_issues: bool = False


@dataclass(frozen=True)
class AgentConfig:
    max_concurrent_issues: int = 2
    max_attempts: int = 3
    normal_continue_delay_ms: int = 1000
    max_retry_backoff_ms: int = 300000


@dataclass(frozen=True)
class PiConfig:
    command: str = "pi"
    provider: str | None = None
    model: str | None = None
    default_thinking: str = "medium"
    implementation_thinking: str = "high"
    use_subagents: bool | str = True
    session_root: str | None = None
    resources_dir: str = ".pi"
    resources_required: bool = False


@dataclass(frozen=True)
class PromptConfig:
    implement: str = "prompts/pi-implement.md"
    rework: str = "prompts/pi-rework.md"
    review_context: str = "prompts/autoreview-context.md"


@dataclass(frozen=True)
class GithubConfig:
    use_gh_cli_auth: bool = True
    ready_label: str = "ai:ready"
    claimed_label: str = "ai:claimed"
    blocked_label: str = "ai:blocked"
    rework_label: str = "ai:rework"
    review_label: str = "ai:review"
    merge_ready_label: str = "ai:merge-ready"
    failed_label: str = "ai:failed"
    done_label: str = "ai:done"
    ignore_label: str = "ai:ignore"
    human_review_label: str = "human-review"


@dataclass(frozen=True)
class GithubOutputCommentsConfig:
    claim: bool = False
    normal_transition: bool = False
    rework_scheduled: bool = True
    blocked: bool = True
    pr_ready: bool = True
    done: bool = False


@dataclass(frozen=True)
class GithubOutputPrBodyConfig:
    include_changed_files: bool = False
    include_safety_section: bool = False
    include_task_ids: bool = False
    include_logs: bool = False
    include_validation: bool = True


@dataclass(frozen=True)
class GithubOutputIssueBodyConfig:
    template: str = "compact"
    include_agent_instructions: bool = False
    include_workflow_rules: bool = False


@dataclass(frozen=True)
class GithubOutputConfig:
    style: str = "compact"
    issue_body_max_chars: int = 1200
    pr_body_max_chars: int = 900
    comment_max_chars: int = 300
    comments: GithubOutputCommentsConfig = field(default_factory=GithubOutputCommentsConfig)
    pr_body: GithubOutputPrBodyConfig = field(default_factory=GithubOutputPrBodyConfig)
    issue_body: GithubOutputIssueBodyConfig = field(default_factory=GithubOutputIssueBodyConfig)


@dataclass(frozen=True)
class GeneratedDiffExceptionConfig:
    path: str
    verify_command: str


@dataclass(frozen=True)
class PolicyConfig:
    require_clean_base: bool = True
    never_push_to_main: bool = True
    require_tests_before_pr: bool = True
    require_independent_review: bool = True
    require_autoreview_before_merge: bool = True
    require_autoreview_before_pr: bool = True
    auto_merge: bool = False
    max_changed_files_without_human_review: int = 20
    max_diff_lines_without_human_review: int = 1500
    shared_integration_files: list[str] = field(default_factory=list)
    generated_diff_exceptions: list[GeneratedDiffExceptionConfig] = field(default_factory=list)


@dataclass(frozen=True)
class AutoreviewConfig:
    enabled: bool = True
    command: str = "autoreview"
    engine: str = "codex"
    mode: str = "branch"
    base: str = "origin/main"
    require_clean: bool = True
    advisory: bool = True
    mandatory_gate: bool = True


@dataclass(frozen=True)
class KanbanConfig:
    board_slug: str = "pyqtgraph-to-cpp"
    board_scope: str = "project"
    tenant_strategy: str = "tags"
    default_tenant: str = "cpp"
    tenant_label_prefix: str = "tenant:"
    tag_label_prefix: str = "tag:"


@dataclass(frozen=True)
class ValidationConfig:
    commands: list[str] = field(default_factory=lambda: ["python3 -m pytest -q"])
    diff_check: bool = True


@dataclass(frozen=True)
class WorkflowConfig:
    tracker: TrackerConfig
    workspace: WorkspaceConfig
    body: str = ""
    agent: AgentConfig = field(default_factory=AgentConfig)
    pi: PiConfig = field(default_factory=PiConfig)
    github: GithubConfig = field(default_factory=GithubConfig)
    github_output: GithubOutputConfig = field(default_factory=GithubOutputConfig)
    prompts: PromptConfig = field(default_factory=PromptConfig)
    policy: PolicyConfig = field(default_factory=PolicyConfig)
    autoreview: AutoreviewConfig = field(default_factory=AutoreviewConfig)
    kanban: KanbanConfig = field(default_factory=KanbanConfig)
    validation: ValidationConfig = field(default_factory=ValidationConfig)

    @classmethod
    def from_mapping(cls, data: dict[str, Any], body: str = "") -> "WorkflowConfig":
        if not isinstance(data, dict):
            raise ConfigError("workflow front matter must be a mapping")

        tracker_data = _section(data, "tracker")
        workspace_data = _section(data, "workspace")
        agent_data = _optional_section(data, "agent")
        pi_data = _optional_section(data, "pi")
        github_data = _optional_section(data, "github")
        github_output_data = _optional_section(data, "github_output")
        prompts_data = _optional_section(data, "prompts")
        policy_data = _optional_section(data, "policy")
        autoreview_data = _optional_section(data, "autoreview")
        kanban_data = _optional_section(data, "kanban")
        validation_data = _optional_section(data, "validation")

        _require_text(tracker_data.get("repo"), "tracker.repo")
        _require_text(workspace_data.get("root"), "workspace.root")

        config = cls(
            tracker=TrackerConfig(**_dataclass_kwargs(TrackerConfig, tracker_data)),
            workspace=WorkspaceConfig(**_dataclass_kwargs(WorkspaceConfig, workspace_data)),
            body=body,
            agent=AgentConfig(**_dataclass_kwargs(AgentConfig, agent_data)),
            pi=PiConfig(**_dataclass_kwargs(PiConfig, pi_data)),
            github=GithubConfig(**_dataclass_kwargs(GithubConfig, github_data)),
            github_output=_github_output_config(github_output_data),
            prompts=PromptConfig(**_dataclass_kwargs(PromptConfig, prompts_data)),
            policy=_policy_config(policy_data),
            autoreview=AutoreviewConfig(**_dataclass_kwargs(AutoreviewConfig, autoreview_data)),
            kanban=KanbanConfig(**_dataclass_kwargs(KanbanConfig, kanban_data)),
            validation=ValidationConfig(**_dataclass_kwargs(ValidationConfig, validation_data)),
        )
        config.validate()
        return config

    def validate(self) -> None:
        _require_text(self.tracker.repo, "tracker.repo")
        if self.tracker.kind != "github":
            raise ConfigError("tracker.kind must be github")
        _require_text(self.workspace.root, "workspace.root")
        if self.workspace.strategy != "git-worktree":
            raise ConfigError("workspace.strategy must be git-worktree")
        _require_text(self.workspace.base_branch, "workspace.base_branch")
        if self.agent.max_concurrent_issues < 1:
            raise ConfigError("agent.max_concurrent_issues must be >= 1")
        if self.agent.max_attempts < 1:
            raise ConfigError("agent.max_attempts must be >= 1")
        _require_text(self.pi.command, "pi.command")
        if self.pi.provider is not None:
            _require_text(self.pi.provider, "pi.provider")
        if self.pi.model is not None:
            _require_text(self.pi.model, "pi.model")
        for name in ("default_thinking", "implementation_thinking"):
            if getattr(self.pi, name) not in {"off", "minimal", "low", "medium", "high", "xhigh"}:
                raise ConfigError(f"pi.{name} must be one of: off, minimal, low, medium, high, xhigh")
        if self.pi.use_subagents not in {True, "auto"}:
            raise ConfigError("pi.use_subagents must be true or auto")
        _require_text(self.pi.resources_dir, "pi.resources_dir")
        if not isinstance(self.pi.resources_required, bool):
            raise ConfigError("pi.resources_required must be boolean")
        for name in ("implement", "rework", "review_context"):
            _require_text(getattr(self.prompts, name), f"prompts.{name}")
        for name in ("ready_label", "claimed_label", "blocked_label", "rework_label", "review_label", "merge_ready_label", "failed_label", "done_label", "ignore_label", "human_review_label"):
            _require_text(getattr(self.github, name), f"github.{name}")
        if self.github_output.style != "compact":
            raise ConfigError("github_output.style must be compact")
        if self.github_output.issue_body.template != "compact":
            raise ConfigError("github_output.issue_body.template must be compact")
        for name in ("issue_body_max_chars", "pr_body_max_chars", "comment_max_chars"):
            if int(getattr(self.github_output, name)) < 1:
                raise ConfigError(f"github_output.{name} must be >= 1")
        if not isinstance(self.policy.auto_merge, bool):
            raise ConfigError("policy.auto_merge must be boolean")
        if self.policy.never_push_to_main is not True:
            raise ConfigError("policy.never_push_to_main must be true")
        if self.policy.auto_merge:
            if self.policy.require_independent_review is not True:
                raise ConfigError("policy.auto_merge requires policy.require_independent_review=true")
            if self.policy.require_autoreview_before_merge is not True:
                raise ConfigError("policy.auto_merge requires policy.require_autoreview_before_merge=true")
            if self.policy.require_autoreview_before_pr is not True:
                raise ConfigError("policy.auto_merge requires policy.require_autoreview_before_pr=true")
        if not isinstance(self.policy.generated_diff_exceptions, list):
            raise ConfigError("policy.generated_diff_exceptions must be a list")
        for index, item in enumerate(self.policy.generated_diff_exceptions):
            _require_text(item.path, f"policy.generated_diff_exceptions[{index}].path")
            _require_text(item.verify_command, f"policy.generated_diff_exceptions[{index}].verify_command")
        if not isinstance(self.policy.shared_integration_files, list) or not all(isinstance(path, str) and path.strip() for path in self.policy.shared_integration_files):
            raise ConfigError("policy.shared_integration_files must be a list of non-empty path strings")
        if self.autoreview.enabled is not True:
            raise ConfigError("autoreview.enabled must be true")
        if self.autoreview.advisory is not True or self.autoreview.mandatory_gate is not True:
            raise ConfigError("autoreview must be configured as an advisory mandatory gate")
        _require_text(self.autoreview.command, "autoreview.command")
        _require_text(self.kanban.board_slug, "kanban.board_slug")
        if self.kanban.board_scope != "project":
            raise ConfigError("kanban.board_scope must be project")
        if self.kanban.tenant_strategy != "tags":
            raise ConfigError("kanban.tenant_strategy must be tags")
        _require_text(self.kanban.default_tenant, "kanban.default_tenant")
        _require_text(self.kanban.tenant_label_prefix, "kanban.tenant_label_prefix")
        _require_text(self.kanban.tag_label_prefix, "kanban.tag_label_prefix")
        if not isinstance(self.validation.commands, list) or not all(isinstance(cmd, str) for cmd in self.validation.commands):
            raise ConfigError("validation.commands must be a list of shell command strings")
        if self.policy.auto_merge:
            if self.validation.diff_check is not True:
                raise ConfigError("policy.auto_merge requires validation.diff_check=true")
            if not any(command.strip() for command in self.validation.commands):
                raise ConfigError("policy.auto_merge requires at least one validation command")


@dataclass(frozen=True)
class LoadedWorkflow:
    config: WorkflowConfig
    path: Path
    repo_root: Path


def load_workflow(path: str | Path = "WORKFLOW.md") -> WorkflowConfig:
    text = Path(path).read_text(encoding="utf-8")
    return parse_workflow_text(text)


def validate_workflow_contract(path: str | Path = "WORKFLOW.md") -> list[str]:
    """Return static lean-workflow contract violations for a WORKFLOW.md file."""
    workflow_path = Path(path).resolve()
    errors: list[str] = []
    try:
        config = load_workflow(workflow_path)
    except (ConfigError, OSError, TypeError) as exc:
        return [str(exc)]

    root = workflow_path.parent
    required_files = {"WORKFLOW.md": workflow_path, "AGENTS.md": root / "AGENTS.md"}
    for label, file_path in required_files.items():
        if not file_path.exists():
            errors.append(f"{label} does not exist: {file_path}")

    prompt_fields = {
        "prompts.implement": config.prompts.implement,
        "prompts.rework": config.prompts.rework,
        "prompts.review_context": config.prompts.review_context,
    }
    for field_name, relative_path in prompt_fields.items():
        prompt_path = _contract_path(root, relative_path)
        if not prompt_path.exists():
            errors.append(f"{field_name} file does not exist: {prompt_path}")
            continue
        text = prompt_path.read_text(encoding="utf-8")
        if not _prompt_has_authority_boundaries(text):
            errors.append(f"{field_name} must prohibit commit/push/merge/PR creation: {prompt_path}")

    resources_path = _contract_path(root, config.pi.resources_dir)
    if config.pi.resources_required and not resources_path.exists():
        errors.append(f"pi.resources_dir does not exist: {resources_path}")
    if resources_path.exists():
        errors.extend(_validate_pi_resources(resources_path))

    if config.policy.auto_merge and config.policy.never_push_to_main is not True:
        errors.append("policy.auto_merge requires policy.never_push_to_main=true")
    if config.policy.auto_merge and config.policy.require_independent_review is not True:
        errors.append("policy.auto_merge requires policy.require_independent_review=true")
    if config.policy.auto_merge and config.policy.require_autoreview_before_merge is not True:
        errors.append("policy.auto_merge requires policy.require_autoreview_before_merge=true")
    if config.policy.auto_merge and config.policy.require_autoreview_before_pr is not True:
        errors.append("policy.auto_merge requires policy.require_autoreview_before_pr=true")
    if config.policy.auto_merge and config.validation.diff_check is not True:
        errors.append("policy.auto_merge requires validation.diff_check=true")
    if config.policy.auto_merge and not any(command.strip() for command in config.validation.commands):
        errors.append("policy.auto_merge requires at least one validation command")
    if config.autoreview.enabled is not True or config.autoreview.mandatory_gate is not True:
        errors.append("autoreview must be enabled with mandatory_gate=true")
    if config.autoreview.advisory is not True:
        errors.append("autoreview.advisory must remain true while mandatory_gate enforces release")
    return errors


def _contract_path(root: Path, value: str) -> Path:
    path = Path(value).expanduser()
    if not path.is_absolute():
        path = root / path
    return path


def _prompt_has_authority_boundaries(text: str) -> bool:
    lower = " ".join(text.lower().split())
    if "do not" not in lower:
        return False
    return all(word in lower for word in ("commit", "push", "merge")) and ("pr" in lower or "pull request" in lower)


def _validate_pi_resources(resources_path: Path) -> list[str]:
    errors: list[str] = []
    agents_dir = resources_path / "agents"
    if agents_dir.exists():
        for file_path in sorted(agents_dir.glob("*")):
            if not file_path.is_file():
                continue
            text = file_path.read_text(encoding="utf-8").lower()
            name = file_path.name.lower()
            if any(role in name for role in ("scout", "review")) and not ("read-only" in text or "read only" in text):
                errors.append(f"Pi resource {file_path} must mark scout/reviewer roles read-only")
            if "implement" in name and ("read-only" in text or "read only" in text) and "write" not in text:
                errors.append(f"Pi resource {file_path} should leave implementer write-capable")
    return errors


def load_workflow_with_context(path: str | Path = "WORKFLOW.md") -> LoadedWorkflow:
    workflow_path = Path(path).resolve()
    return LoadedWorkflow(
        config=load_workflow(workflow_path),
        path=workflow_path,
        repo_root=workflow_path.parent,
    )


def parse_workflow_text(text: str) -> WorkflowConfig:
    front_matter, body = split_front_matter(text)
    try:
        data = yaml.safe_load(front_matter) or {}
    except yaml.YAMLError as exc:
        raise ConfigError(f"invalid YAML front matter: {exc}") from exc
    return WorkflowConfig.from_mapping(data, body=body)


def split_front_matter(text: str) -> tuple[str, str]:
    marker = "---\n"
    if not text.startswith(marker):
        raise ConfigError("WORKFLOW.md must start with YAML front matter")
    end = text.find("\n---\n", len(marker))
    if end == -1:
        raise ConfigError("WORKFLOW.md front matter must end with ---")
    front_matter = text[len(marker) : end]
    body = text[end + len("\n---\n") :]
    return front_matter, body


def _section(data: dict[str, Any], name: str) -> dict[str, Any]:
    value = data.get(name, {})
    if value is None:
        return {}
    if not isinstance(value, dict):
        raise ConfigError(f"{name} section must be a mapping")
    return value


def _optional_section(data: dict[str, Any], name: str) -> dict[str, Any]:
    value = data.get(name, {})
    if value is None:
        return {}
    if not isinstance(value, dict):
        raise ConfigError(f"{name} section must be a mapping")
    return value


def _dataclass_kwargs(cls: type, data: dict[str, Any]) -> dict[str, Any]:
    allowed = {field.name for field in fields(cls)}
    return {key: value for key, value in data.items() if key in allowed}


def _policy_config(data: dict[str, Any]) -> PolicyConfig:
    raw_exceptions = data.get("generated_diff_exceptions", [])
    if raw_exceptions is None:
        raw_exceptions = []
    if not isinstance(raw_exceptions, list):
        raise ConfigError("policy.generated_diff_exceptions must be a list")
    exceptions: list[GeneratedDiffExceptionConfig] = []
    for index, item in enumerate(raw_exceptions):
        if not isinstance(item, dict):
            raise ConfigError(f"policy.generated_diff_exceptions[{index}] must be a mapping")
        exceptions.append(
            GeneratedDiffExceptionConfig(
                path=item.get("path", ""),
                verify_command=item.get("verify_command", ""),
            )
        )
    top_level = {key: value for key, value in data.items() if key != "generated_diff_exceptions"}
    return PolicyConfig(
        **_dataclass_kwargs(PolicyConfig, top_level),
        generated_diff_exceptions=exceptions,
    )


def _github_output_config(data: dict[str, Any]) -> GithubOutputConfig:
    nested_keys = {"comments", "pr_body", "issue_body"}
    top_level = {key: value for key, value in data.items() if key not in nested_keys}
    comments_data = _child_section(data, "comments")
    pr_body_data = _child_section(data, "pr_body")
    issue_body_data = _child_section(data, "issue_body")
    return GithubOutputConfig(
        **_dataclass_kwargs(GithubOutputConfig, top_level),
        comments=GithubOutputCommentsConfig(**_dataclass_kwargs(GithubOutputCommentsConfig, comments_data)),
        pr_body=GithubOutputPrBodyConfig(**_dataclass_kwargs(GithubOutputPrBodyConfig, pr_body_data)),
        issue_body=GithubOutputIssueBodyConfig(**_dataclass_kwargs(GithubOutputIssueBodyConfig, issue_body_data)),
    )


def _child_section(data: dict[str, Any], name: str) -> dict[str, Any]:
    value = data.get(name, {})
    if value is None:
        return {}
    if not isinstance(value, dict):
        raise ConfigError(f"github_output.{name} section must be a mapping")
    return value


def _require_text(value: object, field_name: str) -> None:
    if not isinstance(value, str) or not value.strip():
        raise ConfigError(f"{field_name} is required")
