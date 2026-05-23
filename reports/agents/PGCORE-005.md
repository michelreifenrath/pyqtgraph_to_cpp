# PGCORE-005 Implementation Report

## Summary
Implemented PyQtGraph-aligned `mkColor` and `intColor` helpers using native C++/Qt `QColor` APIs. The implementation covers short PyQtGraph color names, Qt named colors, short and long hex strings with alpha, grayscale floats, RGB/RGBA channel inputs, non-finite channel coercion to 0, QColor copy behavior, integer-index colors, and pair-style `(index, hues)` forwarding for sequence inputs.

## Rework update
Addressed the autoreview findings with a bounded patch:
- removed the prior out-of-scope CMake/CMake-options changes from the combined final diff;
- avoided the QtTest-gated production-symbol path by not changing CMake in this issue scope;
- changed `intColor` hue/value step math to use Python-style floor division for negative descending ranges.

## Files changed
- `include/pyqtgraph/functions.hpp` - public `pyqtgraph::mkColor` and `pyqtgraph::intColor` declarations plus C++ array/tuple convenience overloads.
- `src/pyqtgraph/functions.cpp` - color conversion implementation adapted from PyQtGraph `functions.py` at `pyqtgraph-0.14.0` / commit `a20028b98294b9cc8770f2015a92eb342224b788`, including floor-division semantics for `intColor`.
- `tests/core/test_mkColor.cpp` - focused native C++ test program covering fixture behavior, invalid-input exceptions, and descending `intColor` hue/value ranges.
- `reports/agents/PGCORE-005.md` - this report.

## Validation
- `c++ -std=c++20 -fPIC -Iinclude src/pyqtgraph/functions.cpp tests/core/test_mkColor.cpp $(pkg-config --cflags --libs Qt6Gui Qt6Core) -o /tmp/pyqtgraph_cpp_test_mkColor && /tmp/pyqtgraph_cpp_test_mkColor` - exit 0.
- `git diff --check` - exit 0.
- `scripts/gate focus` - exit 0.
- `scripts/gate commit` - exit 0.
- `python3 -m pytest -q` - exit 0, 171 passed.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` - exit 0.

## Notes
- `scripts/gate focus PGCORE-005` was not rerun because the repository gate CLI accepts only the gate mode; `scripts/gate focus` is the supported focused gate invocation.
- No PR was opened, and no commit/push/merge was performed, per handoff instructions.
