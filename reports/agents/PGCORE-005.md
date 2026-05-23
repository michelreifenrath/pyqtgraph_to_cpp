# PGCORE-005 Implementation Report

## Summary
Implemented PyQtGraph-aligned `mkColor` and `intColor` helpers using native C++/Qt `QColor` APIs. The implementation covers short PyQtGraph color names, Qt named colors, short and long hex strings with alpha, grayscale floats, RGB/RGBA channel inputs, non-finite channel coercion to 0, QColor copy behavior, integer-index colors, and pair-style `(index, hues)` forwarding for sequence inputs.

## Rework update
Addressed the latest autoreview finding with a bounded build-integration patch:
- added `src/pyqtgraph/functions.cpp` to the `pyqtgraph_cpp` library target;
- propagated `Qt6::Core` and `Qt6::Gui` through the public library target because `functions.hpp` exposes Qt color/string types;
- registered `tests/core/test_mkColor.cpp` as `pyqtgraph_cpp.core.mkColor` in CTest and linked the focused test with `Qt6::Gui`.

## Files changed
- `include/pyqtgraph/functions.hpp` - public `pyqtgraph::mkColor` and `pyqtgraph::intColor` declarations plus C++ array/tuple convenience overloads.
- `src/pyqtgraph/functions.cpp` - color conversion implementation adapted from PyQtGraph `functions.py` at `pyqtgraph-0.14.0` / commit `a20028b98294b9cc8770f2015a92eb342224b788`, including floor-division semantics for `intColor`.
- `tests/core/test_mkColor.cpp` - focused native C++ test program covering fixture behavior, invalid-input exceptions, and descending `intColor` hue/value ranges.
- `CMakeLists.txt` - wires `functions.cpp` into `pyqtgraph_cpp` and registers the focused mkColor CTest executable.
- `cmake/PyQtGraphCppOptions.cmake` - includes QtGui in the Qt discovery gate used by the new public QColor API.
- `reports/agents/PGCORE-005.md` - this report.

## Validation
- `cmake --preset dev` - exit 0.
- `cmake --build --preset dev` - exit 0.
- `ctest --preset dev -R pyqtgraph_cpp.core.mkColor --output-on-failure` - exit 0.
- `ctest --test-dir build/dev -N` - lists `pyqtgraph_cpp.core.mkColor`.
- `git diff --check` - exit 0.
- `scripts/gate focus` - exit 0.
- `scripts/gate commit` - exit 0.
- `python3 -m pytest -q` - exit 0, 171 passed.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` - exit 0.

## Notes
- `scripts/gate focus PGCORE-005` was not rerun because the repository gate CLI accepts only the gate mode; `scripts/gate focus` is the supported focused gate invocation.
- No PR was opened, and no commit/push/merge was performed, per handoff instructions.
