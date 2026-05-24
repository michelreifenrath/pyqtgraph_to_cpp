# PGCORE-006 Implementation Report

## Summary
- Added public `pyqtgraph::mkPen` and `pyqtgraph::mkBrush` helpers in `include/pyqtgraph/functions.hpp`.
- Implemented non-template overloads in `src/pyqtgraph/functions.cpp`, delegating color normalization to existing `mkColor` and preserving `std::invalid_argument` failures for invalid color inputs.
- Added focused C++ coverage in `tests/core/test_mkPen_mkBrush.cpp` for defaults, nullptr no-pen/no-brush behavior, copy overloads, representative color delegation, attributes/styles, RGB/RGBA overloads, and invalid inputs.

## Behavior Notes
- `mkPen()` returns light gray (`mkColor("l")` / RGBA `200,200,200,255`), width `1.0`, `Qt::SolidLine`, cosmetic `true`.
- `mkPen(nullptr)` returns the default light-gray cosmetic pen with its style set to `Qt::NoPen`, matching PyQtGraph's observable default attributes.
- `mkBrush(nullptr)` returns a `QBrush` with `Qt::NoBrush`.
- No no-argument `mkBrush()` overload was added.
- `const char*` null input remains distinct from `nullptr` overloads and throws through `mkColor`.

## Validation
- `cmake --preset dev` — passed.
- `cmake --build --preset dev --target pyqtgraph_cpp` — passed.
- `c++ -std=c++20 -Iinclude tests/core/test_mkPen_mkBrush.cpp build/dev/libpyqtgraph_cpp.a $(pkg-config --cflags --libs Qt6Core Qt6Gui) -o /tmp/pyqtgraph_cpp_core_mkpen_mkbrush` — passed.
- `/tmp/pyqtgraph_cpp_core_mkpen_mkbrush` — passed.
- `python3 oracle/scripts/generate_numeric_oracles.py --check` — passed (`numeric oracles verified (2 cases)`).
- `git diff --check` and `git diff --cached --check` — passed.
- `scripts/gate commit` — passed (exit code 0).
- `python3 -m pytest -q` — passed (`188 passed`).
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — passed (`workflow valid`).
- `scripts/gate focus PGCORE-006` — failed with exit code 2 because the gate accepts mode arguments only and rejects the extra issue id.
- `scripts/gate focus` — passed (exit code 0; runs `python3 -m pytest -q`).
- Rework: `cmake --build --preset dev --target pyqtgraph_cpp_core_mkpen_mkbrush` — passed.
- Rework: `ctest --test-dir build/dev -N -R 'pyqtgraph_cpp.core.mkPen_mkBrush'` — passed and listed `Test #3: pyqtgraph_cpp.core.mkPen_mkBrush`.
- Rework: `ctest --test-dir build/dev --output-on-failure -R 'pyqtgraph_cpp.core.mkPen_mkBrush'` — passed.

## Scope Notes / Risks
- No-diff rework gate: this pass leaves this report update as the minimal reviewable git change because the only listed finding was that the prior rework completed with a clean worktree.
- No PR was opened from this Pi run because the workflow/user instructions prohibit commit, push, and merge; Hermes/release automation owns PR creation after handoff.
- Rework added the minimal `CMakeLists.txt` test registration required by autoreview so `tests/core/test_mkPen_mkBrush.cpp` is now built as `pyqtgraph_cpp_core_mkpen_mkbrush` and registered with CTest as `pyqtgraph_cpp.core.mkPen_mkBrush`.
- Subagent scratch handoff files were removed from the worktree before final handoff.
- The hand-authored mkPen/mkBrush JSON oracle fixture was removed from `oracle/fixtures/numeric` because that directory is generator-managed and rejects unknown JSON fixtures in this issue scope.
