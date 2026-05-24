# PGCORE-009 Implementation Report

## Summary
Implemented public NaN-aware floating-point extrema helpers `pyqtgraph::nanmin` and `pyqtgraph::nanmax` for `std::span<const float>`, `std::span<const double>`, and `std::span<const long double>`, plus floating-point initializer-list convenience overloads. The helpers skip only NaN values, preserve infinities as valid extrema, and return quiet NaN for empty or all-NaN input.

## Files changed
Net PGCORE-009 branch diff after the bounded rework is limited to:
- `include/pyqtgraph/functions.hpp` - added `std::span`-based public declarations, floating-point initializer-list forwarding overloads, and conditional Qt color declarations so numeric-only consumers can include the header without Qt color headers.
- `src/pyqtgraph/functions.cpp` - added shared NaN-skipping extrema implementation and public overload definitions; Qt color implementation remains conditionally compiled when Qt color headers are available.
- `tests/core/test_nan_minmax.cpp` - added focused no-framework C++ coverage for finite values, mixed NaNs, infinities, all-NaN and empty input, float and long double return types, and initializer-list convenience calls.
- `CMakeLists.txt` - builds `src/pyqtgraph/functions.cpp` into the core library unconditionally and registers the numeric-only `pyqtgraph_cpp_core_nan_minmax` CTest target outside the Qt color gate.
- `reports/agents/PGCORE-009.md` - this report.

## Validation
- `cmake -S . -B /tmp/pgcore009-noqt-build -DPYQTGRAPH_CPP_REQUIRE_QT=OFF -DPYQTGRAPH_CPP_REQUIRE_OPENCV=OFF -DBUILD_TESTING=ON -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON` - exit 0.
- `cmake --build /tmp/pgcore009-noqt-build --target pyqtgraph_cpp_core_nan_minmax --parallel` - exit 0.
- `ctest --test-dir /tmp/pgcore009-noqt-build -R pyqtgraph_cpp.core.nan_minmax --output-on-failure` - exit 0.
- `git diff --check` - exit 0.
- `scripts/gate focus --timeout 1800` - exit 0.
- `scripts/gate commit --timeout 1800` - exit 0.
- `python3 -m pytest -q` - exit 0; 204 passed.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` - exit 0.

## Notes/Risks
- Rework was limited to PGCORE-009 owned files. The final bounded pass restored unrelated CMake target wiring to the branch state and added explicit `long double` nanmin/nanmax test coverage so the handoff contains a concrete issue-scoped diff.
- Earlier rework addressed the autoreview CMake integration findings: no-Qt builds now link `nanmin`/`nanmax`, and normal CTest discovery now includes the focused numeric test.
