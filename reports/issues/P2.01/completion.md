# P2.01 completion report

## Scope
- Implemented pinned Point/Vector oracle proof for PyQtGraph 0.14.0 at commit `a20028b98294b9cc8770f2015a92eb342224b788`.
- Added named Point power equivalence helpers for upstream `Point.__pow__` / `Point.__rpow__`.
- No visual validation required: P2.01 covers core numeric/API semantics only.

## Artifacts
- Oracle generator: `oracle/scripts/generate_P2_01_point_vector_oracle.py`
- Oracle fixture: `oracle/fixtures/P2_01/point_vector_semantics.json`
- C++ oracle comparison: `tests/oracle/P2_01_point_vector_oracle_comparison.cpp`
- RED checkpoint: `reports/issues/P2.01/red_failure.txt`

## C++ equivalences documented by fixture
- Python len/index/iteration for Point and Vector are represented by `coordinateCount()`, `at()`, `set()`, and `operator[]` where available; no range iterators were added.
- Upstream Point power is represented by `Point::pow(QPointF)`, `Point::pow(double)`, and `pyqtgraph::pow(double, const Point&)`; `operator^` remains intentionally unsupported because it is bitwise/precedence-misleading in C++.
- Python-only dynamic coercions and invalid positional arities that are impossible in statically typed C++ are covered by typed Qt constructors and initializer-list length validation.
- Python `ZeroDivisionError` from zero-length `Point.norm()` is represented by `std::domain_error` in C++ and verified against the fixture-recorded upstream error.

## TDD RED checkpoint
Command:
```sh
cmake --build --preset dev --target pyqtgraph_cpp_oracle_P2_01 --parallel
```
Exit code: 2

Expected failure summary: the new P2.01 oracle target failed to compile because `pyqtgraph::Point` had no `pow` member and namespace `pyqtgraph` had no reflected `pow` helper. Full summary: `reports/issues/P2.01/red_failure.txt`.

## Final validation
- `python3 oracle/scripts/generate_P2_01_point_vector_oracle.py --check` -> exit 0; fixture current.
- `cmake --preset dev` -> exit 0.
- `cmake --build --preset dev --parallel` -> exit 0.
- `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P2.01 --output-on-failure` -> exit 0; 3/3 P2.01 tests passed.
- `git diff --check` -> exit 0.
- `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp` -> exit 1 due pre-existing unrelated blocked-by metadata references including P0.02/P0.08/P1.06/P1.04/P1.01/P1.03/P0.01; no code scope broadened.

## Rework validation
- `python3 -m automation.pi_symphony.cli run-issue --workflow /home/michel/code/pyqtgraph_to_cpp/WORKFLOW.md --issue 120 --phase review --json` -> exit 1; autoreview found the oracle accepted the fixture-recorded zero-length `Point.norm()` upstream error as a NaN C++ equivalence.
- Rework aligned `Point().norm()` with the fixture by throwing `std::domain_error` and updated focused C++/oracle checks accordingly.
- `python3 oracle/scripts/generate_P2_01_point_vector_oracle.py --check` -> exit 0; fixture current.
- `cmake --preset dev` -> exit 0.
- `cmake --build --preset dev --parallel` -> exit 0.
- `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P2.01 --output-on-failure` -> exit 0; 3/3 P2.01 tests passed.
- `QT_QPA_PLATFORM=offscreen ctest --preset dev --output-on-failure` -> exit 0; 33/33 tests passed.
- `python3 -m pytest -q` -> exit 0; 301 passed.
- `git diff --check` -> exit 0.
- `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp` -> exit 1 due pre-existing unrelated blocked-by metadata references including P0.02/P0.08/P1.06/P1.04/P1.01/P1.03/P0.01/P0.06; no code scope broadened.
- `git diff --name-only origin/main...HEAD` -> exit 0; returned the branch-scope paths listed below.

## Changed-file scope check
Modified paths are limited to Point/Vector implementation/tests, P2.01 oracle/fixture/report artifacts, and narrow CMake P2.01 target/label wiring. No `WORKFLOW.md` or automation policy files were changed.

Branch-scope paths:
- `CMakeLists.txt`
- `include/pyqtgraph/Point.hpp`
- `oracle/fixtures/P2_01/point_vector_semantics.json`
- `oracle/scripts/generate_P2_01_point_vector_oracle.py`
- `reports/issues/P2.01/completion.md`
- `reports/issues/P2.01/red_failure.txt`
- `src/pyqtgraph/Point.cpp`
- `tests/core/test_Point.cpp`
- `tests/core/test_Vector.cpp`
- `tests/oracle/P2_01_point_vector_oracle_comparison.cpp`
