# Issue 120 P2.01 worker handoff

Implemented P2.01 Point/Vector oracle proof and Point power helpers.

## Changed files
- `include/pyqtgraph/Point.hpp` / `src/pyqtgraph/Point.cpp`: added `Point::pow(QPointF)`, `Point::pow(double)`, and reflected `pyqtgraph::pow(double, const Point&)`.
- `tests/core/test_Point.cpp`: added Point power checks and coordinate sequence-equivalence checks.
- `tests/core/test_Vector.cpp`: added Vector coordinate sequence-equivalence checks.
- `CMakeLists.txt`: added `pyqtgraph_cpp_oracle_P2_01` and P2.01 labels for oracle/Point/Vector tests.
- `oracle/scripts/generate_P2_01_point_vector_oracle.py`: pinned upstream Point/Vector oracle generator.
- `oracle/fixtures/P2_01/point_vector_semantics.json`: generated fixture with upstream version/commit and tolerances.
- `tests/oracle/P2_01_point_vector_oracle_comparison.cpp`: C++ fixture comparison target.
- `reports/issues/P2.01/red_failure.txt`: expected RED failure evidence.
- `reports/issues/P2.01/completion.md`: final validation/report.

## TDD RED summary
Command: `cmake --build --preset dev --target pyqtgraph_cpp_oracle_P2_01 --parallel`
Exit code: 2
Expected failure: missing `Point::pow(...)` and `pyqtgraph::pow(...)`. See `reports/issues/P2.01/red_failure.txt`.

## Final validation
- `python3 oracle/scripts/generate_P2_01_point_vector_oracle.py --check` -> exit 0.
- `cmake --preset dev` -> exit 0.
- `cmake --build --preset dev --parallel` -> exit 0.
- `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P2.01 --output-on-failure` -> exit 0, 3/3 tests passed.
- `git diff --check` -> exit 0.
- `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp` -> exit 1 due unrelated proposed-issue blocked-by metadata (recorded in completion report); no code scope broadened.

## Caveats
- Upstream `Point(0,0).norm()` raises `ZeroDivisionError`; existing C++ returns NaN components. The fixture records the upstream error and the C++ oracle keeps the existing NaN behavior under the approved minimal P2.01 scope.
- No out-of-scope production or automation-policy files were changed.
