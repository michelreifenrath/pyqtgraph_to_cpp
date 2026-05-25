# Implementation Plan

## Goal
Make `pyqtgraph-cpp` installable and consumable through CMake `find_package(pyqtgraph-cpp CONFIG REQUIRED)` with exported targets, public headers, and a focused downstream package-consumer proof for issue P1.01 / #105.

## Tasks
1. **Confirm issue-owned scope before editing**
   - File: none
   - Changes: Run `git diff --name-only origin/main...HEAD` and compare any intended changes against the owned files: `CMakeLists.txt`, `cmake/**`, install/export target files, and focused proof/report files under `reports/issues/P1.01/**`.
   - Acceptance: No planned production changes outside owned build/package plumbing. Stop before editing if implementation requires source, header API, tests outside `reports/issues/P1.01/**`, or workflow files.

2. **Add the package-consumer proof first**
   - File: `reports/issues/P1.01/consumer/CMakeLists.txt`
   - Changes: Create or update a minimal downstream CMake project that calls `find_package(pyqtgraph-cpp CONFIG REQUIRED)`, builds `p1_01_package_consumer`, links `pyqtgraph_cpp::pyqtgraph_cpp`, requires C++20, enables CTest, and registers a `P1.01.package-consumer` test labeled `P1.01`.
   - Acceptance: The fixture configures only when an installed package config/export exists and exposes the imported target.

3. **Add the downstream smoke executable**
   - File: `reports/issues/P1.01/consumer/main.cpp`
   - Changes: Include installed public headers such as `<pyqtgraph/core/ArrayView.hpp>` and `<pyqtgraph/PlotData.hpp>`, instantiate a small `ArrayView`/`PlotData` scenario, and assert basic compile/link/runtime behavior without depending on build-tree-only paths.
   - Acceptance: The executable proves public include directories and the installed library target are usable from a separate consumer build.

4. **Record the expected pre-implementation failure**
   - File: `reports/issues/P1.01/package-consumer-preimplementation.md`
   - Changes: From a clean state before CMake install/export support, record commands and failure output for:
     - `rm -rf build/install-P1_01 build/consumer-P1_01`
     - `cmake --preset release -DCMAKE_INSTALL_PREFIX="$PWD/build/install-P1_01"`
     - `cmake --build --preset release --target install --parallel`
     - `cmake -S reports/issues/P1.01/consumer -B build/consumer-P1_01 -DCMAKE_PREFIX_PATH="$PWD/build/install-P1_01"`
   - Acceptance: The report captures the expected failure, typically missing `install` target, missing `pyqtgraph-cppConfig.cmake`, or missing exported `pyqtgraph_cpp::pyqtgraph_cpp` target. If the current branch already contains the implementation, do not fabricate failure output; preserve existing factual report or note that the failure was already recorded.

5. **Enable CMake install/package helper modules**
   - File: `CMakeLists.txt`
   - Changes: Ensure the top-level file includes `GNUInstallDirs` and `CMakePackageConfigHelpers` after `CMAKE_MODULE_PATH` is set and before package configuration/install commands are used.
   - Acceptance: Configure succeeds and install destinations use standard variables such as `${CMAKE_INSTALL_LIBDIR}`, `${CMAKE_INSTALL_BINDIR}`, and `${CMAKE_INSTALL_INCLUDEDIR}`.

6. **Make installed include usage explicit on the library target**
   - File: `CMakeLists.txt`
   - Changes: Ensure `target_include_directories(pyqtgraph_cpp PUBLIC ...)` uses build/install generator expressions:
     - `$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>`
     - `$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>`
   - Acceptance: Exported targets do not contain source-tree include paths and downstream consumers include headers from the install prefix.

7. **Derive public Qt package dependencies for the config file**
   - File: `CMakeLists.txt`
   - Changes: Build `_pyqtgraph_cpp_public_qt_components` from the same feature flags used to link public Qt targets: append `Core` when `_pyqtgraph_cpp_has_qtcore`, `Gui` when vector or color support is enabled, and `Widgets` when graphics item support is enabled; remove duplicates; assign `PYQTGRAPH_CPP_PACKAGE_QT_COMPONENTS` for config substitution.
   - Acceptance: The generated package config requires exactly the Qt components needed by exported public link interfaces and does not hard-code components that were not enabled at configure time.

8. **Add the installed package config template**
   - File: `cmake/pyqtgraph-cppConfig.cmake.in`
   - Changes: Create a config template containing `@PACKAGE_INIT@`, `include(CMakeFindDependencyMacro)`, substitution of `@PYQTGRAPH_CPP_PACKAGE_QT_COMPONENTS@`, conditional `find_dependency(Qt6 6 COMPONENTS ...)`, and inclusion of `${CMAKE_CURRENT_LIST_DIR}/pyqtgraph-cppTargets.cmake`.
   - Acceptance: A downstream `find_package(pyqtgraph-cpp CONFIG REQUIRED)` resolves public Qt dependencies before loading exported targets.

9. **Generate and install config/version files**
   - File: `CMakeLists.txt`
   - Changes: Set `_pyqtgraph_cpp_package_dir` to `${CMAKE_INSTALL_LIBDIR}/cmake/pyqtgraph-cpp`; call `configure_package_config_file()` to generate `pyqtgraph-cppConfig.cmake`; call `write_basic_package_version_file()` to generate `pyqtgraph-cppConfigVersion.cmake` using `${PROJECT_VERSION}` and `SameMajorVersion` compatibility; install both generated files to the package directory.
   - Acceptance: `cmake --build --preset release --target install` installs `lib/cmake/pyqtgraph-cpp/pyqtgraph-cppConfig.cmake` and `pyqtgraph-cppConfigVersion.cmake` under the chosen prefix.

10. **Install library target, options target, headers, and export set**
    - File: `CMakeLists.txt`
    - Changes: Add `install(TARGETS pyqtgraph_cpp pyqtgraph_cpp_project_options EXPORT pyqtgraph-cppTargets ...)` with archive/library/runtime destinations; install `include/` headers matching `*.hpp`; add `install(EXPORT pyqtgraph-cppTargets FILE pyqtgraph-cppTargets.cmake NAMESPACE pyqtgraph_cpp:: DESTINATION "${_pyqtgraph_cpp_package_dir}")`.
    - Acceptance: The install tree contains `lib/libpyqtgraph_cpp.a` or the platform equivalent, public headers under `include/`, and `lib/cmake/pyqtgraph-cpp/pyqtgraph-cppTargets.cmake` exposing `pyqtgraph_cpp::pyqtgraph_cpp`.

11. **Validate the install/export implementation with a clean package-consumer run**
    - File: `reports/issues/P1.01/package-consumer.md`
    - Changes: Record exact commands and relevant output for this clean validation sequence:
      ```sh
      rm -rf build/install-P1_01 build/consumer-P1_01
      cmake --preset release -DCMAKE_INSTALL_PREFIX="$PWD/build/install-P1_01"
      cmake --build --preset release --target install --parallel
      cmake -S reports/issues/P1.01/consumer -B build/consumer-P1_01 -DCMAKE_PREFIX_PATH="$PWD/build/install-P1_01"
      cmake --build build/consumer-P1_01 --parallel
      ctest --test-dir build/consumer-P1_01 --output-on-failure
      ```
    - Acceptance: Configure, install, downstream configure, downstream build, and downstream CTest all pass. The report should include enough output to prove the installed config/export path was used.

12. **Run repository validation and ownership checks**
    - File: `reports/issues/P1.01/package-consumer.md`
    - Changes: Append results for:
      ```sh
      scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
      git diff --check
      git diff --name-only origin/main...HEAD
      ```
    - Acceptance: `git diff --check` passes. `git diff --name-only origin/main...HEAD` lists only owned files. If `scripts/check_proposed_issues` fails only because of known unrelated `P0.01` blocked-by references, record that caveat with exact output instead of treating it as an implementation failure.

13. **Avoid duplicate work if the branch already has P1.01 support**
    - File: `CMakeLists.txt`, `cmake/pyqtgraph-cppConfig.cmake.in`, `reports/issues/P1.01/**`
    - Changes: If these files already contain the install/export/config/consumer proof described above, do not rewrite them for style-only reasons. Re-run validation and update only factual report artifacts if they are stale and within `reports/issues/P1.01/**`.
    - Acceptance: Final diff remains minimal and issue-scoped.

## Files to Modify
- `CMakeLists.txt` - add/verify package helper includes, installed include interface, public Qt component derivation, config/version generation, target/header installs, and export set installation.
- `cmake/pyqtgraph-cppConfig.cmake.in` - installed package config template that finds public Qt dependencies and imports exported targets.
- `reports/issues/P1.01/consumer/CMakeLists.txt` - focused downstream consumer proof using `find_package` and `pyqtgraph_cpp::pyqtgraph_cpp`.
- `reports/issues/P1.01/consumer/main.cpp` - compile/link/runtime smoke for installed public headers and target.
- `reports/issues/P1.01/package-consumer-preimplementation.md` - expected failure evidence before implementation, if not already factually recorded.
- `reports/issues/P1.01/package-consumer.md` - passing install/export/downstream consumer proof and validation notes.

## New Files
- `cmake/pyqtgraph-cppConfig.cmake.in` - create only if absent; package config template for installed consumers.
- `reports/issues/P1.01/consumer/CMakeLists.txt` - create only if absent; downstream proof fixture.
- `reports/issues/P1.01/consumer/main.cpp` - create only if absent; downstream smoke executable.
- `reports/issues/P1.01/package-consumer-preimplementation.md` - create only if absent; preimplementation failure report.
- `reports/issues/P1.01/package-consumer.md` - create only if absent; final validation report.

## Dependencies
- Task 1 must happen before any edit.
- Tasks 2 and 3 must happen before Task 4 so the expected failure tests the final consumer contract.
- Task 4 must happen before implementation Tasks 5-10 to satisfy TDD evidence, unless factual preimplementation evidence already exists.
- Tasks 5 and 6 must happen before Tasks 9 and 10 because package generation/install relies on standard install variables and correct target interfaces.
- Task 7 must happen before Task 8/9 package config generation so Qt dependency substitution is correct.
- Tasks 8-10 together are required before Task 11 can pass.
- Task 12 depends on Task 11 and is the final validation/ownership gate.
- Task 13 applies throughout to keep changes minimal when existing implementation is already present.

## Risks
- The current checkout may already contain the intended P1.01 implementation and proof artifacts; in that case, do not duplicate or churn files, and focus on validation/report freshness.
- Do not fabricate preimplementation failure logs if the implementation is already present on the working branch; use existing factual report evidence or stop for direction if the issue requires fresh TDD evidence from a clean base.
- Exported Qt dependencies must match the actually linked public targets. Missing `find_dependency(Qt6 ...)` calls can make downstream configure fail; over-requiring disabled Qt components can make valid minimal installs unusable.
- `pyqtgraph_cpp_project_options` is exported as an imported interface target because `pyqtgraph_cpp` links it publicly; removing it from the export set would break consumers.
- Generated exported targets may repeat `Qt6::Gui` if multiple enabled features publicly link Gui. This is noisy but not expected to break consumers; avoid broad refactors unless required for validation.
- `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp` may fail for pre-existing unrelated blocked-by metadata such as missing local `P0.01`; record exact output and escalate only if the failure is new or related to P1.01.
- Stop and escalate if validation requires editing unowned source/header/test/workflow files, changing public API outside build plumbing, or adding compatibility shims/fallbacks not requested by issue #105.
