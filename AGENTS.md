# Agent Instructions

Use this file as the first local guide for AI agents working in this repository.

## Source of truth

Follow guidance in this order:

1. The assigned GitHub issue, especially its Scope, Owned files, TDD plan, visual-validation requirements, and Done definition.
2. `MISSION.md` for product goal, scope, non-goals, and hard invariants.
3. `FACTORY_RULES.md` for issue readiness, scope budgets, evidence, validation, auto-merge, attribution, and protected-file policy.
4. `AGENTS.md` for repository-wide agent behavior.
5. `WORKFLOW.md` for runtime automation and safety policy.

If these conflict, obey the narrower issue-owned scope and stop for human direction before expanding scope.

## Porting rules

- Build a native C++ library, not a Python wrapper.
- Use Qt/C++ for GUI and rendering work.
- Use OpenCV/C++ math and data structures instead of NumPy-style Python dependencies.
- Keep PyQtGraph class names, object names, and example names aligned with upstream unless the issue explicitly says otherwise; the C++ library branding, include root, and namespace are CppQtGraph/`cppqtgraph`.
- Read the pinned upstream PyQtGraph source when behavior or naming matters.
- If behavior is unclear, write or request a PyQtGraph oracle probe before guessing.

## Owned files and TDD

- Edit only files listed in the issue's Owned files section, plus narrowly required shared integration files allowed by `FACTORY_RULES.md` and `WORKFLOW.md`.
- Do not touch `WORKFLOW.md`, automation policy, source, examples, reports, or other files unless the issue owns them.
- For behavior changes, add or update a focused failing test before production implementation, then make it pass.
- Run focused tests and the configured validation from `WORKFLOW.md` before handing off when practical.

## Visual validation

For pixel-affecting work, follow the visual-validation level in the issue and `FACTORY_RULES.md`. Generate required artifacts and request GPT-5.5 semantic visual review when required. If required visual evidence cannot be produced inside owned files, stop and escalate.
