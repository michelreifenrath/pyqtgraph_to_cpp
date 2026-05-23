# PGCORE-005 Implementation Report

## Summary
Implemented PyQtGraph-aligned `mkColor` and `intColor` helpers using native C++/Qt `QColor` APIs. The implementation covers short PyQtGraph color names, Qt named colors, short and long hex strings with alpha, grayscale floats, RGB/RGBA channel inputs, non-finite channel coercion to 0, QColor copy behavior, integer-index colors, and pair-style `(index, hues)` forwarding for sequence inputs.

## Rework update
Addressed the latest autoreview findings with a bounded CMake scope/opt-out patch:
- reverted the unowned `cmake/PyQtGraphCppOptions.cmake` changes from the branch diff;
- kept the issue-owned `CMakeLists.txt` integration, but now adds `src/pyqtgraph/functions.cpp` and public `Qt6::Core`/`Qt6::Gui` links only when the Qt mkColor dependencies are available;
- registers `tests/core/test_mkColor.cpp` only behind the same guard, so `PYQTGRAPH_CPP_REQUIRE_QT=OFF` can still configure without Qt targets.

## Files changed
- `include/pyqtgraph/functions.hpp` - public `pyqtgraph::mkColor` and `pyqtgraph::intColor` declarations plus C++ array/tuple convenience overloads.
- `src/pyqtgraph/functions.cpp` - color conversion implementation adapted from PyQtGraph `functions.py` at `pyqtgraph-0.14.0` / commit `a20028b98294b9cc8770f2015a92eb342224b788`, including floor-division semantics for `intColor`.
- `tests/core/test_mkColor.cpp` - focused native C++ test program covering fixture behavior, invalid-input exceptions, and descending `intColor` hue/value ranges.
- `CMakeLists.txt` - conditionally wires `functions.cpp` and the focused mkColor CTest executable when Qt color targets are available.
- `reports/agents/PGCORE-005.md` - this report.

## Validation
- `git diff --check` - exit 0.
- `git diff --exit-code origin/main -- cmake/PyQtGraphCppOptions.cmake` - exit 0.
- `cmake -S . -B build/issue-24-no-qt -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=TRUE -DPYQTGRAPH_CPP_REQUIRE_QT=OFF -DPYQTGRAPH_CPP_REQUIRE_OPENCV=OFF -DBUILD_TESTING=ON` - exit 0.
- `cmake --build build/issue-24-no-qt` - exit 0.
- `ctest --test-dir build/issue-24-no-qt --output-on-failure` - exit 0, 2 tests passed with Qt discovery disabled.
- `cmake -S . -B build/issue-24-qt-on -DBUILD_TESTING=ON` - exit 0.
- `cmake --build build/issue-24-qt-on --target pyqtgraph_cpp_core_mkcolor` - exit 0.
- `ctest --test-dir build/issue-24-qt-on --output-on-failure -R mkColor` - exit 0, 1 test passed.
- `scripts/gate focus` - exit 0.
- `scripts/gate commit` - exit 0.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` - exit 0.

## Notes
- `scripts/gate focus PGCORE-005` was not rerun because the repository gate CLI accepts only the gate mode; `scripts/gate focus` is the supported focused gate invocation.
- No PR was opened, and no commit/push/merge was performed, per handoff instructions.
