# Implementation Plan

## Goal
Create the PGBOOT-001 C++ repository skeleton with a CMake 3.26+/C++20 baseline, guarded Qt6/OpenCV discovery, empty test wiring, and documented validation while staying within the issue-owned files.

## Tasks
1. **Confirm the current owned skeleton state before editing**: Run a no-build initial check to establish the expected TDD-style baseline and ownership boundaries.
   - File: no file changes
   - Changes: Inspect `git status --short`, confirm only the owned skeleton paths are modified/untracked, and optionally run `git diff --check` before implementation to catch whitespace errors without creating build artifacts.
   - Acceptance: Status shows work limited to `CMakeLists.txt`, `CMakePresets.json`, `cmake/`, `include/pyqtgraph/.gitkeep`, `src/pyqtgraph/.gitkeep`, `tests/CMakeLists.txt`, and `tests/smoke/test_empty.cpp`; no build directories or reports are created.

2. **Finalize the top-level CMake project skeleton**: Ensure the project declares the supported build baseline and wires only shared configuration and tests.
   - File: `CMakeLists.txt`
   - Changes: Keep `cmake_minimum_required(VERSION 3.26)`, `project(pyqtgraph_cpp VERSION 0.1.0 LANGUAGES CXX)`, C++20 required/no extensions, local `cmake/` module path, includes for `PyQtGraphCppOptions`, `PyQtGraphCppWarnings`, and `PyQtGraphCppSanitizers`, `pyqtgraph_cpp_configure_options()`, `pyqtgraph_cpp_project_options` interface target with `cxx_std_20`, `pyqtgraph_cpp_project_warnings` creation, `include(CTest)`, and `add_subdirectory(tests)` gated by `BUILD_TESTING`.
   - Acceptance: `cmake --preset dev` reaches dependency discovery and test configuration without missing-module or missing-target errors when required dependencies are installed.

3. **Finalize developer CMake presets**: Make the default developer preset represent the strict PGBOOT-001 dependency baseline.
   - File: `CMakePresets.json`
   - Changes: Keep a `dev` configure preset using Ninja if currently present, Debug build type, `BUILD_TESTING=ON`, `PYQTGRAPH_CPP_REQUIRE_QT=ON`, `PYQTGRAPH_CPP_REQUIRE_OPENCV=ON`, and a matching build/test preset named `dev` that targets the same binary directory.
   - Acceptance: `cmake --preset dev`, `cmake --build --preset dev --parallel`, and `ctest --preset dev --output-on-failure` are the canonical full validation commands. If a machine lacks Qt6/OpenCV, validation may use explicit `-DPYQTGRAPH_CPP_REQUIRE_QT=OFF -DPYQTGRAPH_CPP_REQUIRE_OPENCV=OFF` fallback commands and must document that the full preset is dependency-blocked.

4. **Finalize guarded Qt/OpenCV discovery options**: Keep dependency policy centralized and cache-visible.
   - File: `cmake/PyQtGraphCppOptions.cmake`
   - Changes: Define `PYQTGRAPH_CPP_REQUIRE_QT` default `ON`, `PYQTGRAPH_CPP_REQUIRE_OPENCV` default `ON`, and `PYQTGRAPH_CPP_QT_MAJOR_VERSION` default `6`; use quiet `find_package(Qt6 6 COMPONENTS Core Test QUIET)` and `find_package(OpenCV 4 QUIET)`; set internal cache booleans `PYQTGRAPH_CPP_HAS_QT` and `PYQTGRAPH_CPP_HAS_OPENCV`; issue `FATAL_ERROR` only when the corresponding `REQUIRE_*` option is `ON` and the dependency is missing.
   - Acceptance: Configure fails clearly under the `dev` preset when required Qt6/OpenCV packages are absent; configure succeeds with both `REQUIRE_*` options disabled and exposes false `HAS_*` values for guarded tests.

5. **Finalize compiler warning target helper**: Provide a reusable warnings interface target without applying warnings globally.
   - File: `cmake/PyQtGraphCppWarnings.cmake`
   - Changes: Keep a function such as `pyqtgraph_cpp_create_warnings_target(<target>)` that creates an `INTERFACE` library and applies reasonable compiler-specific warnings via generator/compiler checks.
   - Acceptance: Any test target can link `pyqtgraph_cpp_project_warnings`; configure/build succeeds on common GCC/Clang/MSVC without unknown-option failures.

6. **Finalize sanitizer helper**: Keep sanitizers opt-in and target-scoped.
   - File: `cmake/PyQtGraphCppSanitizers.cmake`
   - Changes: Keep option `PYQTGRAPH_CPP_ENABLE_SANITIZERS` default `OFF`; provide `pyqtgraph_cpp_enable_sanitizers(<target>)` that no-ops unless enabled and applies address/undefined sanitizer compile/link flags only for supported non-MSVC compilers.
   - Acceptance: Default builds are unaffected; enabling the option on supported compilers adds sanitizer flags to smoke targets only.

7. **Finalize smoke test wiring**: Keep a dependency-free CTest smoke target and optionally add a guarded Qt Test smoke only if reviewers require explicit Qt Test coverage.
   - File: `tests/CMakeLists.txt`
   - Changes: Keep `pyqtgraph_cpp_smoke_empty` built from `smoke/test_empty.cpp`, link it to `pyqtgraph_cpp_project_options` and `pyqtgraph_cpp_project_warnings`, call `pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_smoke_empty)`, and register `add_test(NAME pyqtgraph_cpp.smoke.empty COMMAND pyqtgraph_cpp_smoke_empty)`. If adding Qt Test is authorized and desired, add a second executable only inside `if(PYQTGRAPH_CPP_HAS_QT)` and link `Qt6::Core`/`Qt6::Test`; do not make dependency-free fallback depend on Qt.
   - Acceptance: `ctest` always has at least one smoke test when `BUILD_TESTING=ON`; with required dependencies installed, the dev preset builds and runs the test suite.

8. **Finalize the empty C++ smoke test**: Keep the first test intentionally minimal.
   - File: `tests/smoke/test_empty.cpp`
   - Changes: Keep a valid C++20 translation unit with `int main() { return 0; }`; avoid including Qt/OpenCV so the skeleton-only fallback remains testable.
   - Acceptance: The smoke executable compiles and exits successfully under CTest.

9. **Preserve placeholder library directories**: Keep include/source roots present without introducing API surface in PGBOOT-001.
   - File: `include/pyqtgraph/.gitkeep`
   - Changes: Leave as an empty placeholder.
   - Acceptance: `include/pyqtgraph/` exists for future public headers.

10. **Preserve placeholder source directory**: Keep source root present without introducing implementation classes in PGBOOT-001.
    - File: `src/pyqtgraph/.gitkeep`
    - Changes: Leave as an empty placeholder.
    - Acceptance: `src/pyqtgraph/` exists for future implementation files.

11. **Validate the completed skeleton**: Run the required validation contract after implementation.
    - File: no file changes expected, except local ignored build artifacts if CMake creates them.
    - Changes: Execute `cmake --preset dev`; `cmake --build --preset dev --parallel`; `ctest --preset dev --output-on-failure`; `python3 -m pytest -q`; `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md`; `git diff --check`; and, if present and executable, `scripts/run_autoreview --mode commit`.
    - Acceptance: All available commands pass. If `scripts/run_autoreview` is unavailable because that later bootstrap file is not owned/present, document the safe unavailable state rather than creating or editing it. If Qt6/OpenCV are missing, document the full-preset dependency failure and run the skeleton-only fallback configure/build/ctest with both dependency requirements disabled.

## Files to Modify
- `CMakeLists.txt` - top-level CMake 3.26/C++20 project, shared option/warning/sanitizer targets, and `BUILD_TESTING` test subdirectory wiring.
- `CMakePresets.json` - developer configure/build/test presets for the strict Qt6/OpenCV baseline.
- `cmake/PyQtGraphCppOptions.cmake` - guarded Qt6 Core/Test and OpenCV 4 discovery with `REQUIRE_*` and `HAS_*` variables.
- `cmake/PyQtGraphCppWarnings.cmake` - reusable target-scoped compiler warnings helper.
- `cmake/PyQtGraphCppSanitizers.cmake` - opt-in target-scoped sanitizer helper.
- `tests/CMakeLists.txt` - smoke executable and CTest registration, with optional guarded Qt Test target only if explicitly accepted.
- `tests/smoke/test_empty.cpp` - dependency-free empty smoke executable.
- `include/pyqtgraph/.gitkeep` - preserve placeholder include directory.
- `src/pyqtgraph/.gitkeep` - preserve placeholder source directory.

## New Files
- None. Do not add `reports/agents/PGBOOT-001.md` in this issue unless ownership is separately authorized.

## Dependencies
- Tasks 2-6 must be completed before Task 7 so test targets can link the shared option/warning/sanitizer targets.
- Task 7 depends on Task 8 for the smoke test source.
- Tasks 9-10 are independent placeholders but should remain within ownership.
- Task 11 depends on all implementation tasks and on local availability of Qt6, OpenCV 4, Python test dependencies, and optional `scripts/run_autoreview`.

## Risks
- `reports/agents/PGBOOT-001.md` is requested by the generic Done definition but is not in the owned-file list; do not plan or perform that edit without explicit authorization.
- The `dev` preset intentionally requires Qt6 and OpenCV 4, so it may fail on lightweight machines. Treat that as an environment dependency and validate with the documented skeleton-only fallback only when necessary.
- A Qt Test executable may be expected by reviewers because the workflow mentions Qt Test; current scope permits an empty/smoke Qt Test or CTest target. Prefer the dependency-free CTest smoke unless explicit feedback requires adding a guarded Qt Test target.
- Later bootstrap artifacts such as `scripts/gate`, `scripts/run_autoreview`, and `ownership.yaml` are outside PGBOOT-001 ownership; do not create them here.
