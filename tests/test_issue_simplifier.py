from automation.pi_symphony.issue_simplifier import compact_issue_body, simplified_labels


LONG_BODY = """## Goal
Add ArrayView skeleton and tests.

This issue is part of the step-by-step roadmap. Long automation prose should disappear.

## Dependencies
- #1 (`PGBOOT-001`)
- #4 (`PGBOOT-004`)

Automation readiness: Do not add `ai:ready` until all dependencies above are completed.

## Owned files
The agent may edit only:
- `include/pyqtgraph/core/ArrayView.hpp`
- `src/pyqtgraph/core/ArrayView.cpp`
- `tests/core/test_ArrayView.cpp`
- `CMakeLists.txt`

## Scope
Implement:
- Add a minimal non-owning ArrayView template skeleton.
- Support construction from pointer plus shape for 1D data.

Do not implement:
- unrelated PyQtGraph classes, examples, or phases;
- broad refactors outside the owned files.

## TDD plan
Failing tests/checks to add or exercise first:
- [ ] `scripts/gate focus PGCORE-001`
- [ ] `scripts/gate commit`

## Validation commands
```bash
scripts/gate focus PGCORE-001
scripts/gate commit
```

Also run, unless the issue explains why not applicable:

```bash
python3 -m pytest -q
python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md
```

## Done definition
- [ ] tests/checks fail before implementation where applicable;
- [ ] tests/checks pass after implementation;
- [ ] implementation remains within owned files;
- [ ] PR opened by automation or handoff explains why no PR was opened.
"""


def test_compact_issue_body_preserves_operational_sections_without_boilerplate():
    body = compact_issue_body("[AI] PGCORE-001: Add ArrayView skeleton and tests", LONG_BODY, max_chars=1200)

    assert len(body) <= 1200
    assert body.startswith("## Goal\nAdd ArrayView skeleton and tests.")
    assert "## Dependencies" in body
    assert "#1" in body and "#4" in body
    assert "## Owned files" in body
    assert "include/pyqtgraph/core/ArrayView.hpp" in body
    assert "## Validation" in body
    assert "scripts/gate focus PGCORE-001" in body
    assert "python3 -m pytest -q" in body
    assert "## Done" in body
    assert "Long automation prose" not in body
    assert "Do not implement" not in body
    assert "Automation readiness" not in body


def test_compact_issue_body_is_idempotent():
    body = compact_issue_body("[AI] PGCORE-001: Add ArrayView skeleton and tests", LONG_BODY, max_chars=1200)

    assert compact_issue_body("[AI] PGCORE-001: Add ArrayView skeleton and tests", body, max_chars=1200) == body


def test_simplified_labels_keep_state_and_one_domain_label():
    labels = simplified_labels(
        "[AI] PGBOOT-004: Add gate and autoreview wrappers",
        ["ai:claimed", "tenant:cpp", "tag:build", "tag:bootstrap", "ai:rework"],
    )

    assert labels == ["ai:claimed", "ai:rework", "tag:bootstrap"]


def test_simplified_labels_use_examples_domain_for_example_issue():
    labels = simplified_labels(
        "[AI] PGEXAMPLE-001: Port SimplePlot.cpp and validate smoke",
        ["tenant:cpp", "tag:plot", "tag:examples"],
    )

    assert labels == ["tag:examples"]
