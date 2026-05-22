# Port Ticket Prompt

You are porting PyQtGraph to C++ in `michelreifenrath/pyqtgraph_to_cpp`.

Follow `AGENTS.md`, `WORKFLOW.md`, and `docs/pyqtgraph-cpp-port-workflow.md` strictly. Work only on the assigned GitHub issue and edit only files listed in its Owned files section.

## Implementation rules

- Use test-driven development; for behavior changes, add or update a focused failing test first and confirm it fails for the expected reason.
- Preserve upstream PyQtGraph class names, file names, object hierarchy, and example names unless the issue says otherwise.
- Use Qt/C++ for GUI and rendering.
- Use OpenCV/C++ math and data structures instead of Python wrappers or NumPy.
- Read relevant upstream PyQtGraph source, examples, and existing tests before coding.
- If behavior is unclear, add or request a PyQtGraph oracle probe before guessing.
- Validate affected examples when relevant.
- For pixel-affecting work, declare the visual-validation level, generate required `reports/visual-diffs/<case>/` artifacts when owned by the issue, and request GPT-5.5 semantic visual review when `gpt_visual_review` is `required_for_pr`.
- Do not push to `main`, merge, or commit manually unless explicitly asked.

## Before coding

1. Read the full issue body, especially Goal, Owned files, Scope, TDD plan, Visual validation, Validation commands, and Done definition.
2. Confirm every intended edit is inside Owned files.
3. Read `AGENTS.md`, `WORKFLOW.md`, and `docs/pyqtgraph-cpp-port-workflow.md`.
4. Read upstream PyQtGraph source and affected examples/tests when relevant.
5. Add or update tests first for behavior changes.
6. Confirm the focused test fails for the expected reason.

## After coding

1. Run focused tests for the changed behavior.
2. Run affected examples when relevant.
3. Run configured validation from `WORKFLOW.md` when practical.
4. Run `git diff --check` when practical.
5. Run `scripts/gate commit` only if that script exists; otherwise use current configured gates.
6. Run `scripts/run_autoreview --mode commit` only if that script exists; otherwise rely on the configured review/autoreview flow.
7. Write required agent reports only when the issue owns the report path; otherwise note the ownership conflict in your summary.
8. Summarize changed files, validation, unresolved risks, and skipped Done-definition items.
