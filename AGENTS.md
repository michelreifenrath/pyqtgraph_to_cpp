# Agent Instructions

Use this file as the local guide for AI agents working in this repository.

## Source of truth

Follow guidance in this order:

1. The user's current request.
2. `MISSION.md` for product goal, non-goals, hard invariants, and lightweight example-first strategy.
3. `examples/example_manifest.yaml` for active example order and status.
4. `AGENTS.md` for repository-wide agent behavior.
5. Existing code/tests/build files for local conventions.

If these conflict, obey the narrower user-approved scope and stop for human direction before expanding scope.

## Porting rules

- Build a native C++ library, not a Python wrapper.
- Do not introduce a Python runtime dependency for the library or C++ examples.
- Use Qt/C++ for GUI, rendering, events, timers, and signals.
- Use OpenCV/STL/small explicit helpers instead of cloning NumPy behavior.
- Keep useful PyQtGraph class names, object names, and example names aligned with upstream unless the task explicitly authorizes divergence.
- Keep C++ branding, include root, package name, and namespace as CppQtGraph/`cppqtgraph`.
- Read the pinned upstream PyQtGraph source when behavior or naming matters.
- If behavior is unclear, write or request a PyQtGraph oracle probe before guessing.

## Lightweight development rules

- Port example-first: add only the native behavior needed by the current example or focused behavior slice.
- Do not build broad subsystems, placeholder APIs, or speculative compatibility layers before an example needs them.
- Preserve existing implemented C++ code unless the approved task directly requires a narrow behavior change.
- Prefer small, coherent edits over broad rewrites.
- Keep generated/local artifacts out of the active tree unless they are intentional fixtures or documentation.

## TDD and validation

- For behavior changes, add or update a focused failing test or oracle first when practical, then make it pass.
- For pixel-affecting work, provide screenshot/visual-diff evidence or explain why visual validation is not applicable.
- Use deterministic fixtures for random data and numeric expectations.
- Run focused tests plus appropriate local validation before handing off.
- Recommended baseline checks are:

```bash
python3 -m pytest -q
cmake --preset dev
cmake --build --preset dev --parallel
ctest --preset dev --output-on-failure
git diff --check
```

- `scripts/validate_local` runs this baseline in one command; use `--preset ci-linux` in CI environments.

## Reference and oracle policy

- The pinned PyQtGraph reference is `pyqtgraph-0.14.0` at commit `a20028b98294b9cc8770f2015a92eb342224b788`.
- PyQtGraph may be used to inspect upstream behavior, generate deterministic fixtures, render reference screenshots, and validate interactions.
- PyQtGraph must not be used at runtime by the C++ library or shipped examples.
