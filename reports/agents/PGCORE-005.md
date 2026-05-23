# PGCORE-005 Implementation Report

## Summary
Implemented PyQtGraph-aligned `mkColor` and `intColor` helpers using native C++/Qt `QColor` APIs. The implementation covers short PyQtGraph color names, Qt named colors, short and long hex strings with alpha, grayscale floats, RGB/RGBA channel inputs, non-finite channel coercion to 0, QColor copy behavior, integer-index colors, and pair-style `(index, hues)` forwarding for sequence inputs.

## Rework update
Addressed autoreview integration findings by wiring the color implementation into the normal CMake build and removing the unmanaged numeric fixture conflict.

## Files changed
- `include/pyqtgraph/functions.hpp` - public `pyqtgraph::mkColor` and `pyqtgraph::intColor` declarations plus C++ array/tuple convenience overloads.
- `src/pyqtgraph/functions.cpp` - color conversion implementation adapted from PyQtGraph `functions.py` at `pyqtgraph-0.14.0` / commit `a20028b98294b9cc8770f2015a92eb342224b788`.
- `tests/core/test_mkColor.cpp` - focused native C++ test program covering fixture behavior and invalid-input exceptions.
- `CMakeLists.txt` - links `src/pyqtgraph/functions.cpp` into `pyqtgraph_cpp`, propagates Qt Core/Gui usage requirements, and registers the mkColor CTest target.
- `cmake/PyQtGraphCppOptions.cmake` - discovers/requires Qt6 Gui alongside Core/Test for the public QColor API.
- `oracle/fixtures/numeric/mkColor.json` - removed; mkColor color cases are not part of the managed numeric-oracle fixture set.
- `reports/agents/PGCORE-005.md` - this report.

## Validation
- `git diff --check` - exit 0.
- `cmake -S . -B build/dev -DBUILD_TESTING=ON` - exit 0.
- `cmake --build build/dev --target pyqtgraph_cpp_core_mkcolor` - exit 0.
- `ctest --test-dir build/dev -R 'pyqtgraph_cpp\\.core\\.(mkColor|ArrayView)|pyqtgraph_cpp\\.smoke\\.empty' --output-on-failure` - exit 0, 3/3 tests passed.
- `python3 oracle/scripts/generate_numeric_oracles.py --check` - exit 0.
- `scripts/gate focus` - exit 0.
- `scripts/gate commit` - exit 0.
- `python3 -m pytest -q` - exit 0, 171 passed.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` - exit 0.

## Notes
- `scripts/gate focus PGCORE-005` was not rerun because the repository gate CLI accepts only the gate mode; `scripts/gate focus` is the supported focused gate invocation.
- No PR was opened, and no commit/push/merge was performed, per handoff instructions.
