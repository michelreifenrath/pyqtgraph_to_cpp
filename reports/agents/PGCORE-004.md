# PGCORE-004 Implementation Report

## Summary

Added native C++ `pyqtgraph::Vector`, backed by `QVector3D`, for the scoped PyQtGraph Vector behavior: construction, coordinate indexing/mutation, copy, angle, and component-wise absolute value.

## Upstream source consulted

Confirmed pinned PyQtGraph commit `a20028b98294b9cc8770f2015a92eb342224b788` by fetching:

- `pyqtgraph/Vector.py`
- `tests/test_Vector.py`

Observed upstream behavior matched the plan: 2-value inputs set `z = 0`, 3-value inputs preserve `z`, Qt point/size and `QVector3D` inputs are accepted, invalid sequence lengths raise, indexes 0..2 map to x/y/z, `angle()` returns degrees and `None` for zero-length inputs, and `abs(vector)` is component-wise.

## Files changed

- `include/pyqtgraph/Vector.hpp` - public `pyqtgraph::Vector` API and attribution note.
- `src/pyqtgraph/Vector.cpp` - constructors, indexing, angle, abs, and copy implementation.
- `tests/core/test_Vector.cpp` - focused standalone Vector behavior test.
- `CMakeLists.txt` - PGCORE-004 Qt Gui gate, library source wiring, and `pyqtgraph_cpp.core.Vector` CTest registration.
- `reports/agents/PGCORE-004.md` - this report.

## Validation evidence

Final validation:

- `cmake --preset dev` - exit 0.
- `cmake --build build/dev --target pyqtgraph_cpp_core_vector` - exit 0.
- `ctest --test-dir build/dev -R pyqtgraph_cpp.core.Vector --output-on-failure` - exit 0, 1/1 test passed.
- `git diff --check` - exit 0.
- `pytest tests/test_attribution.py` - exit 0, 25 passed.
- `scripts/gate focus` - exit 0 per `.hermes/pi-symphony/logs/gates/focus-summary.json`; `python3 -m pytest -q` reported 208 passed.
- `scripts/gate commit` - exit 0 per `.hermes/pi-symphony/logs/gates/commit-summary.json`; diff checks passed and `python3 -m pytest -q` reported 208 passed.

Failures encountered and fixed during implementation:

- `cmake --build build/dev --target pyqtgraph_cpp_core_vector` initially failed on a test macro argument issue caused by braced commas inside `CHECK(...)`; fixed by assigning the vector to a local variable before the check.
- A later iteration briefly had test/source declarations for out-of-scope `norm()` helpers; removed to keep the API aligned with the approved PGCORE-004 plan, then rebuilt and retested successfully.

## Caveats

- The local `reference/pyqtgraph` checkout was absent, so upstream confirmation used raw files from the pinned GitHub commit.
- `QVector3D` stores float components, so the focused test uses float-appropriate tolerances.
- The C++ API uses `std::optional<double>` for `angle()` to represent upstream Python `None` for zero vectors.
- No numeric oracle fixture changes were made.
