# Code Context

## Files Retrieved
1. `CMakeLists.txt` (lines 1-25) - top-level CMake skeleton currently present as untracked owned file.
2. `CMakePresets.json` (lines 1-34) - developer configure/build/test preset currently present as untracked owned file.
3. `cmake/PyQtGraphCppOptions.cmake` (lines 1-40) - guarded Qt6/OpenCV discovery options currently present as untracked owned file.
4. `cmake/PyQtGraphCppWarnings.cmake` (lines 1-13) - compiler warning target helper currently present as untracked owned file.
5. `cmake/PyQtGraphCppSanitizers.cmake` (lines 1-21) - sanitizer option/helper currently present as untracked owned file.
6. `tests/CMakeLists.txt` (lines 1-13) - current smoke test target wiring.
7. `tests/smoke/test_empty.cpp` (lines 1-4) - current dependency-free empty CTest executable.
8. `docs/pyqtgraph-cpp-port-workflow.md` (lines 19-30, 84-97, 151-190, 307-342, 409-416, 429-449) - canonical C++ port scope, rules, target structure, build baseline, done definition, PGBOOT-001 phase criteria.
9. `WORKFLOW.md` (lines 1-55, 79-86) - automation policy and validation command baseline.

## Key Code

Current branch/status:

```text
## ai/issue-1-ai-pgboot-001-create-c-repo-skeleton-and-c...origin/main
?? CMakeLists.txt
?? CMakePresets.json
?? cmake/
?? include/
?? src/
?? tests/CMakeLists.txt
?? tests/smoke/
```

Current top-level CMake (`CMakeLists.txt:1-25`):

```cmake
cmake_minimum_required(VERSION 3.26)
project(pyqtgraph_cpp VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(PyQtGraphCppOptions)
include(PyQtGraphCppWarnings)
include(PyQtGraphCppSanitizers)
pyqtgraph_cpp_configure_options()
add_library(pyqtgraph_cpp_project_options INTERFACE)
target_compile_features(pyqtgraph_cpp_project_options INTERFACE cxx_std_20)
pyqtgraph_cpp_create_warnings_target(pyqtgraph_cpp_project_warnings)
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
```

Dependency options (`cmake/PyQtGraphCppOptions.cmake:1-40`):

```cmake
option(PYQTGRAPH_CPP_REQUIRE_QT "Require Qt 6 Core and Test for the default PGBOOT-001 test baseline" ON)
option(PYQTGRAPH_CPP_REQUIRE_OPENCV "Require OpenCV 4.x for the default pyqtgraph-cpp baseline" ON)
set(PYQTGRAPH_CPP_QT_MAJOR_VERSION "6" CACHE STRING "Qt major version used by pyqtgraph-cpp")
find_package(Qt6 6 COMPONENTS Core Test QUIET)
find_package(OpenCV 4 QUIET)
# Fatal if required deps are missing; stores PYQTGRAPH_CPP_HAS_QT / _HAS_OPENCV as INTERNAL cache values.
```

Current smoke test wiring (`tests/CMakeLists.txt:1-13`, `tests/smoke/test_empty.cpp:1-4`):

```cmake
add_executable(pyqtgraph_cpp_smoke_empty smoke/test_empty.cpp)
target_link_libraries(pyqtgraph_cpp_smoke_empty PRIVATE pyqtgraph_cpp_project_options pyqtgraph_cpp_project_warnings)
pyqtgraph_cpp_enable_sanitizers(pyqtgraph_cpp_smoke_empty)
add_test(NAME pyqtgraph_cpp.smoke.empty COMMAND pyqtgraph_cpp_smoke_empty)
```

```cpp
int main()
{
    return 0;
}
```

Current owned-path existence:

- Present and untracked: `CMakeLists.txt`, `CMakePresets.json`, `cmake/PyQtGraphCppOptions.cmake`, `cmake/PyQtGraphCppWarnings.cmake`, `cmake/PyQtGraphCppSanitizers.cmake`, `include/pyqtgraph/.gitkeep`, `src/pyqtgraph/.gitkeep`, `tests/CMakeLists.txt`, `tests/smoke/test_empty.cpp`.
- No existing tracked C++ build skeleton found in `origin/main` status; all skeleton files above are untracked in this worktree.

Relevant spec excerpts:

- Goal (`docs/pyqtgraph-cpp-port-workflow.md:19-30`): native C++ library, Qt/C++ primary UI/rendering, OpenCV/C++ math instead of NumPy, tested/validated; not a Python wrapper.
- Rules (`docs/pyqtgraph-cpp-port-workflow.md:84-97`): test-driven, edit only owned files, Qt/C++ primary, OpenCV/C++ data structures, no unauthorized API redesign.
- Target structure (`docs/pyqtgraph-cpp-port-workflow.md:151-190`): includes `CMakeLists.txt`, `CMakePresets.json`, `cmake/PyQtGraphCppOptions.cmake`, warnings/sanitizers modules, `include/pyqtgraph/`, `src/pyqtgraph/`, and `tests/`.
- Build baseline (`docs/pyqtgraph-cpp-port-workflow.md:307-342`): C++20, CMake 3.26+, Qt 6 first, OpenCV 4.x, Qt Test; standard commands `cmake --preset dev`, `cmake --build --preset dev --parallel`, `ctest --preset dev --output-on-failure`; Python checks until `scripts/gate` exists.
- Done definition conflict (`docs/pyqtgraph-cpp-port-workflow.md:409-416`): asks for `reports/agents/<ticket>.md`, but `reports/agents/PGBOOT-001.md` is not in this issue's owned-file list.
- PGBOOT-001 phase (`docs/pyqtgraph-cpp-port-workflow.md:429-449`): creates C++ skeleton/CMake baseline; exit criteria include CMake configures, empty C++ test suite runs, Python automation tests still pass. Some later exit criteria (`scripts/gate`, `run_autoreview`, `ownership.yaml`) belong to later bootstrap issues and are non-owned here.
- Workflow validation (`WORKFLOW.md:39-55`): tests required before PR; validation command is `python3 -m pytest -q`.

## Architecture

The current skeleton is small and CMake-centered:

1. `CMakeLists.txt` establishes project metadata, C++20 requirements, local `cmake/` module path, and includes the options/warnings/sanitizers helpers.
2. `PyQtGraphCppOptions.cmake` owns external dependency discovery and policy:
   - default preset requires Qt6 Core/Test and OpenCV 4;
   - users can opt out with `-DPYQTGRAPH_CPP_REQUIRE_QT=OFF` and/or `-DPYQTGRAPH_CPP_REQUIRE_OPENCV=OFF` for skeleton-only validation;
   - cache internals expose `PYQTGRAPH_CPP_HAS_QT` / `PYQTGRAPH_CPP_HAS_OPENCV` for test gating.
3. `PyQtGraphCppWarnings.cmake` creates an INTERFACE warnings target linked by tests.
4. `PyQtGraphCppSanitizers.cmake` provides `PYQTGRAPH_CPP_ENABLE_SANITIZERS` and per-target sanitizer flag injection.
5. `tests/CMakeLists.txt` currently adds a dependency-free smoke executable and CTest test. It does not yet create a Qt Test based executable even when Qt is available.
6. `include/pyqtgraph/.gitkeep` and `src/pyqtgraph/.gitkeep` keep intended library include/source directories without introducing classes yet.

## Start Here

Start with `CMakePresets.json` and `cmake/PyQtGraphCppOptions.cmake` together. The dev preset currently requires Qt and OpenCV by default, so configure will fail on machines without those packages unless the implementer intentionally validates with overrides. Then open `tests/CMakeLists.txt` to decide whether PGBOOT-001 should stay dependency-free CTest smoke or add a guarded Qt Test executable when `PYQTGRAPH_CPP_HAS_QT` is true.

Likely validation commands:

```bash
# Full baseline, requires Qt6 Core/Test and OpenCV 4 because preset sets both REQUIRE options ON.
cmake --preset dev
cmake --build --preset dev --parallel
ctest --preset dev --output-on-failure

# Skeleton-only fallback if dependencies are not installed.
cmake -S . -B build/dev-skeleton -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DPYQTGRAPH_CPP_REQUIRE_QT=OFF -DPYQTGRAPH_CPP_REQUIRE_OPENCV=OFF
cmake --build build/dev-skeleton --parallel
ctest --test-dir build/dev-skeleton --output-on-failure

# Repository automation baseline from docs/WORKFLOW.
python3 -m pytest -q
python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md
```

No validation commands were run during this scout pass to avoid creating build/test artifacts under the hard `Do not modify files` constraint.

Risks / gaps for planner:

- All owned implementation files already exist but are untracked; planner/implementer should treat them as current worktree state, not baseline from `origin/main`.
- `CMakePresets.json` dev preset requires Qt/OpenCV by default. This matches a strict dependency baseline but may make the default `cmake --preset dev` fail in lightweight CI/dev environments; fallback override command should be documented or the preset/options adjusted if issue expects configure to succeed without deps.
- Scope allows "empty/smoke Qt Test or CTest target"; current implementation is CTest-only, no Qt Test target. If reviewers expect Qt Test specifically because docs list "Qt Test for tests", add a guarded Qt Test smoke only if within owned files and only when Qt is found/required.
- `reports/agents/PGBOOT-001.md` is requested by generic Done definition but is outside owned-file list. Flag this in handoff/review; do not edit it under this issue's ownership constraint.
- Later Phase A exit criteria mention `scripts/gate`, `scripts/run_autoreview`, and `ownership.yaml`, but those are explicitly listed as later PGBOOT issues and are outside current owned files/non-scope.
