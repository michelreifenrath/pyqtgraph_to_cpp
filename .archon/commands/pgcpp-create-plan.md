---
description: Create a narrow implementation plan for a non-bug pgcpp issue.
argument-hint: (reads workflow artifacts)
---

# pgcpp create plan

Use this command for non-bug issues. Do not edit code.

Read:
- `$ARTIFACTS_DIR/issue.json`
- `$ARTIFACTS_DIR/readiness.json`
- `$ARTIFACTS_DIR/research.md`
- Relevant current C++/Qt/OpenCV code, tests/examples, oracle files, and pinned upstream PyQtGraph reference.

Write `$ARTIFACTS_DIR/plan.md` with a minimal, issue-scoped plan:
- Goal and non-goals.
- Owned files/common adjuncts expected to change.
- TDD/oracle-first step, including the specific focused test or fixture.
- Minimal production implementation step.
- Required visual/numeric/oracle artifacts, including GPT-5.5 semantic visual review if required by workflow docs.
- Exact local validation commands.
- Stop conditions for scope ambiguity.

Do not introduce speculative abstractions, wrappers, or DynaChat/web-app concepts. Return a concise summary and the path written.
