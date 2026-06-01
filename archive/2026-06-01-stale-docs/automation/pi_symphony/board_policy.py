from __future__ import annotations

import re
from collections.abc import Iterable

from automation.pi_symphony.config import KanbanConfig

_DEFAULT_BOARD_SLUG = "pyqtgraph-to-cpp"
_FORBIDDEN_PATTERNS = (
    re.compile(r"^issue-\d+$"),
    re.compile(r"^pr-\d+$"),
    re.compile(r"^pi-(worker|reviewer)$"),
)


class BoardPolicyError(ValueError):
    """Raised when a board policy would create per-issue/profile boards."""


def default_board_slug() -> str:
    return _DEFAULT_BOARD_SLUG


def sanitize_slug(value: str) -> str:
    """Derive a lowercase, hyphenated slug from repo/project text."""
    slug = value.strip().lower()
    slug = slug.replace("/", "-").replace("_", "-")
    slug = re.sub(r"[^a-z0-9-]+", "-", slug)
    slug = re.sub(r"-+", "-", slug).strip("-")
    return slug


def validate_board_slug(slug: str) -> str:
    sanitized = sanitize_slug(slug)
    if not sanitized:
        raise BoardPolicyError("kanban.board_slug must not be empty")
    for pattern in _FORBIDDEN_PATTERNS:
        if pattern.fullmatch(sanitized):
            raise BoardPolicyError(
                f"kanban.board_slug {sanitized!r} looks issue/profile/PR-specific; use one repo-level board"
            )
    return sanitized


def normalize_label_value(value: str) -> str:
    """Normalize a tenant/tag label suffix to a stable slug-like value."""
    return sanitize_slug(value)


def derive_tenant(labels: Iterable[str], config: KanbanConfig) -> str:
    for label in labels:
        if label.startswith(config.tenant_label_prefix):
            tenant = normalize_label_value(label.removeprefix(config.tenant_label_prefix))
            if tenant:
                return tenant
    return normalize_label_value(config.default_tenant) or "core"


def derive_tags(labels: Iterable[str], config: KanbanConfig) -> list[str]:
    tags: list[str] = []
    seen: set[str] = set()
    for label in labels:
        if not label.startswith(config.tag_label_prefix):
            continue
        tag = normalize_label_value(label.removeprefix(config.tag_label_prefix))
        if not tag or tag in seen:
            continue
        tags.append(tag)
        seen.add(tag)
    return tags


def build_task_metadata(issue_number: int, labels: Iterable[str], config: KanbanConfig, source_repo: str) -> dict[str, object]:
    return {
        "board_slug": validate_board_slug(config.board_slug),
        "tenant": derive_tenant(labels, config),
        "tags": derive_tags(labels, config),
        "github_issue_number": issue_number,
        "source_repo": source_repo,
        "source": "github-issue",
    }


def hermes_kanban_command(*args: str, config: KanbanConfig | None = None) -> list[str]:
    kanban = config or KanbanConfig()
    slug = validate_board_slug(kanban.board_slug)
    return ["hermes", "kanban", "--board", slug, *[str(arg) for arg in args]]


def board_create_command(config: KanbanConfig | None = None) -> list[str]:
    kanban = config or KanbanConfig()
    slug = validate_board_slug(kanban.board_slug)
    return [
        "hermes",
        "kanban",
        "board",
        "create",
        "--slug",
        slug,
        "--scope",
        kanban.board_scope,
    ]


def standard_commands(config: KanbanConfig | None = None) -> list[list[str]]:
    operations: Iterable[tuple[str, ...]] = (
        ("list",),
        ("stats",),
        ("assignees", "list"),
        ("task", "list"),
    )
    return [hermes_kanban_command(*operation, config=config) for operation in operations]
