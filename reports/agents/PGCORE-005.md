# PGCORE-005 Implementation Report

## Summary
Implemented PyQtGraph-aligned `mkColor` and `intColor` helpers using native C++/Qt `QColor` APIs. The implementation covers short PyQtGraph color names, Qt named colors, short and long hex strings with alpha, grayscale floats, RGB/RGBA channel inputs, non-finite channel coercion to 0, QColor copy behavior, integer-index colors, and pair-style `(index, hues)` forwarding for sequence inputs.

## Files changed
- `include/pyqtgraph/functions.hpp` - public `pyqtgraph::mkColor` and `pyqtgraph::intColor` declarations plus C++ array/tuple convenience overloads.
- `src/pyqtgraph/functions.cpp` - color conversion implementation adapted from PyQtGraph `functions.py` at `pyqtgraph-0.14.0` / commit `a20028b98294b9cc8770f2015a92eb342224b788`.
- `tests/core/test_mkColor.cpp` - focused native C++ test program covering fixture behavior and invalid-input exceptions.
- `oracle/fixtures/numeric/mkColor.json` - deterministic expected RGBA and error fixture cases.
- `reports/agents/PGCORE-005.md` - this report.

## Validation
- `python3 -m json.tool oracle/fixtures/numeric/mkColor.json >/dev/null` - exit 0.
- `c++ -std=c++20 -Iinclude tests/core/test_mkColor.cpp src/pyqtgraph/functions.cpp $(pkg-config --cflags --libs Qt6Core Qt6Gui) -o /tmp/pgcore-005-test_mkColor && QT_QPA_PLATFORM=offscreen /tmp/pgcore-005-test_mkColor` - exit 0.
- `git diff --check` - exit 0; `git diff --check --no-index /dev/null <new-file>` reported no whitespace issues for each new issue file.
- `scripts/gate focus PGCORE-005` - exit 2 because this repository's `scripts/gate` currently accepts only the gate mode and rejects a trailing ticket id.
- `scripts/gate focus` - exit 0.
- `scripts/gate commit` - exit 0.
- `python3 -m pytest -q` - exit 0, 171 passed.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` - exit 0.

## Notes and limitations
- CMake integration was not added because `CMakeLists.txt`, CMake option files, and test registration files are outside the issue-owned file list. The focused test was validated with the ad-hoc compile command instead.
- No PR was opened, and no commit/push/merge was performed, per handoff instructions.
