# PGPLOT-001 PlotData Implementation Report

## Scope
- Added native C++ `pyqtgraph::PlotData` in the issue-owned header/source paths.
- Added focused standalone C++ tests and root CMake wiring.
- No oracle fixture updates were needed; no visual validation is required because this is a data-only helper.

## Upstream reference
- Translated/adapted from PyQtGraph `pyqtgraph/PlotData.py`.
- PyQtGraph ref: `pyqtgraph-0.14.0`.
- Pinned commit: `a20028b98294b9cc8770f2015a92eb342224b788`.

## Behavior and API choices
- C++ value model is intentionally restricted to one-dimensional `double` fields using `std::vector<double>` storage and `std::span<const double>` assignment.
- Upstream `None` default fields are represented as present-but-empty vectors.
- Missing field lookup and extrema throw `std::out_of_range`, the C++ analogue for Python dictionary missing keys.
- Empty present-field extrema throw `std::invalid_argument`, matching NumPy's deterministic failure for empty reductions.
- `min()` / `max()` propagate `NaN` when any field value is `NaN`, matching `np.min` / `np.max`; project nan-skipping helpers are deliberately not used.
- Cached extrema preserve upstream stale-cache behavior: replacing a field after computing `min()` / `max()` does not invalidate cached values.

## Tests added
- Field lifecycle and lookup: new containers, `addFields`, `hasField`, absent field throws, default empty fields, and no overwrite on repeated `addFields`.
- Assignment/replacement and extrema cache behavior, including stale cached extrema after replacement.
- Missing, empty, and NaN extrema behavior.
- Const lookup and mutable indexing coverage.

## Validation
- `cmake --preset dev` — exit 0; configured build in `build/dev`.
- `cmake --build --preset dev --target pyqtgraph_cpp_core_plotdata --parallel` — exit 0; built library and PlotData test target.
- `ctest --preset dev --output-on-failure -R pyqtgraph_cpp.core.PlotData` — exit 0; 1/1 PlotData tests passed.
- `scripts/gate focus PGPLOT-001` — exit 2; local gate CLI does not accept an issue-code argument despite the issue text listing one.
- `scripts/gate focus` — exit 0; ran configured focused workflow validation (`python3 -m pytest -q`).
- `scripts/gate commit` — exit 0; ran diff checks and configured pytest validation.
- `python3 -m pytest -q` — exit 0; 208 passed in 33.33s.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — exit 0; workflow valid.
- `git diff --check` — exit 0; no whitespace errors.

## Risks / notes
- The constrained numeric API is narrower than Python's arbitrary-value container and is documented in the header.
- Stale extrema caches are surprising for C++ users but intentionally match upstream `PlotData.py`.
