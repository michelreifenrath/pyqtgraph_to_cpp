# PGCORE-003 Implementation Report

## Summary

Added a native C++ `pyqtgraph::Point` class backed by `QPointF`, translated/adapted from upstream PyQtGraph `pyqtgraph/Point.py` for the scoped C++ API. The implementation supports point/size/scalar construction, two-value initializer-list construction, coordinate indexing and mutation, component-wise point and scalar arithmetic, reflected scalar operators, compound assignments, and vector helpers (`length`, `norm`, `angle`, `dot`, `cross`, `proj`, `min`, `max`, `copy`, `toQPoint`).

## Files changed

- `include/pyqtgraph/Point.hpp` - public `pyqtgraph::Point` declaration and attribution note.
- `src/pyqtgraph/Point.cpp` - Point constructors, operators, indexing, and vector helper implementation.
- `tests/core/test_Point.cpp` - focused executable test coverage for Point behavior.
- `CMakeLists.txt` - Qt-gated `pyqtgraph_cpp` library target and `pyqtgraph_cpp.core.Point` CTest registration.
- `reports/agents/PGCORE-003.md` - this implementation report.

## Test coverage

The new Point test covers default, scalar, coordinate, Qt point, and Qt size construction; exact two-value initializer-list construction and invalid initializer-list errors; indexed access/mutation and out-of-range errors; component-wise point and scalar arithmetic including reflected scalar operators; compound assignments; copy/value behavior; vector helpers; IEEE division-by-zero behavior; nonzero and zero-vector `norm()` behavior; and `toQPoint()` conversion.

## Validation

- `cmake --preset dev` - passed.
- `cmake --build --preset dev --parallel` - passed.
- `ctest --preset dev --output-on-failure` - passed, 2/2 tests.
- `python3 -m pytest -q` - passed, 130 tests.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` - passed.
- `git diff --check` - passed.
- `git diff --check --no-index /dev/null <new-file>` for each new issue file - passed.
- `scripts/gate focus PGCORE-003` - failed with `gate: error: unrecognized arguments: PGCORE-003`; this repository's gate CLI does not accept a focus target argument.
- `scripts/gate focus` - passed as the supported focused gate invocation.
- `scripts/gate commit` - passed.

During implementation, an intermediate `ctest --preset dev --output-on-failure` run failed before scalar-brace usage in tests/implementation was corrected, and an intermediate `python3 -m pytest -q` run failed until required source attribution notes were added. Final validation passes are listed above; the only remaining validation caveat is the unsupported `scripts/gate focus PGCORE-003` command form.

## Rework resolution

The gate finding `release requires the review phase to complete first` is a workflow-ordering finding, not a Point implementation defect. This bounded rework leaves the Point source and tests unchanged; the release phase should be retried only after the review/autoreview phase records the reviewed head for this rework commit.

## Parity notes and limitations

- `Point::length()` is vector magnitude, matching upstream Point semantics; coordinate count is exposed as `coordinateCount()` rather than Python `__len__`.
- C++ construction is intentionally typed: Qt point/size types, scalar broadcast, and exactly two `double` initializer-list values are supported instead of arbitrary Python sequence duck typing.
- `norm()` intentionally does not special-case the zero vector, preserving normal IEEE floating-point division behavior.
- `angle()` returns degrees by default and radians only when passed `QStringView{u"radians"}`.
- Visual validation is not primary evidence for this non-rendering core geometry class.

## PR status

No PR was opened because this Pi run must not commit, push, merge, or open a pull request.
