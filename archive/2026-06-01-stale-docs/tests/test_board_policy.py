from automation.pi_symphony.board_policy import (
    build_task_metadata,
    derive_tags,
    derive_tenant,
    normalize_label_value,
)
from automation.pi_symphony.config import KanbanConfig


def test_tenant_is_derived_from_tenant_label():
    config = KanbanConfig(default_tenant="core", tenant_label_prefix="tenant:")

    assert derive_tenant(["bug", "tenant:cpp"], config) == "cpp"


def test_tenant_defaults_when_no_tenant_label():
    config = KanbanConfig(default_tenant="core")

    assert derive_tenant(["bug", "tag:parser"], config) == "core"


def test_tags_are_derived_from_tag_labels():
    config = KanbanConfig(tag_label_prefix="tag:")

    assert derive_tags(["tenant:cpp", "tag:parser", "tag:build"], config) == ["parser", "build"]


def test_task_metadata_includes_board_tenant_tags_and_github_source():
    config = KanbanConfig(board_slug="pyqtgraph-to-cpp", default_tenant="core")

    metadata = build_task_metadata(
        issue_number=42,
        labels=["tenant:cpp", "tag:parser", "tag:build"],
        config=config,
        source_repo="michelreifenrath/pyqtgraph_to_cpp",
    )

    assert metadata == {
        "board_slug": "pyqtgraph-to-cpp",
        "tenant": "cpp",
        "tags": ["parser", "build"],
        "github_issue_number": 42,
        "source_repo": "michelreifenrath/pyqtgraph_to_cpp",
        "source": "github-issue",
    }


def test_invalid_tenant_and_tag_labels_are_sanitized_predictably():
    config = KanbanConfig(default_tenant="core")

    assert normalize_label_value(" C++ Parser!! ") == "c-parser"
    assert derive_tenant(["tenant: C++ Parser!!"], config) == "c-parser"
    assert derive_tenant(["tenant: !!!"], config) == "core"
    assert derive_tags(["tag:Build System!!", "tag: ???", "tag:Build System!!"], config) == ["build-system"]
