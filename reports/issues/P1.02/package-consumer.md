# P1.02 package config/version consumer proof

Working directory: `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-106`

## Scope and ownership

- Manifest-expanded target paths: not applicable; this issue has no manifest source or example selectors.
- Shared build/package wiring changed: none. Existing `CMakeLists.txt`/`cmake/pyqtgraph-cppConfig.cmake.in` package export/version support was used as-is.
- Focused P1.02 proof/report artifacts changed: `reports/issues/P1.02/consumer/CMakeLists.txt`; `reports/issues/P1.02/consumer/main.cpp`; `reports/issues/P1.02/package-consumer-preimplementation.md`; `reports/issues/P1.02/package-consumer.md`.
- Manifest/dashboard update: not applicable; no tracked source, class, example, or asset status changed.

## TDD red evidence

See `reports/issues/P1.02/package-consumer-preimplementation.md`.

- Before adding the fixture, the downstream configure command failed because `reports/issues/P1.02/consumer` did not exist.
- After adding the fixture but before installing to `build/install-P1_02`, the same downstream configure failed at `find_package(pyqtgraph-cpp 0.1.0 CONFIG REQUIRED)` because no package config existed in the clean prefix.

## Installed package artifacts

- Install prefix: `build/install-P1_02`
- Library: `build/install-P1_02/lib/libpyqtgraph_cpp.a`
- Package config: `build/install-P1_02/lib/cmake/pyqtgraph-cpp/pyqtgraph-cppConfig.cmake`
- Package version config: `build/install-P1_02/lib/cmake/pyqtgraph-cpp/pyqtgraph-cppConfigVersion.cmake`
- Exported targets: `build/install-P1_02/lib/cmake/pyqtgraph-cpp/pyqtgraph-cppTargets.cmake`
- Exported release targets: `build/install-P1_02/lib/cmake/pyqtgraph-cpp/pyqtgraph-cppTargets-release.cmake`
- Exported consumer target: `pyqtgraph_cpp::pyqtgraph_cpp`
- Downstream consumer binary: `build/consumer-P1_02/p1_02_package_consumer`
- Runtime assets/dependencies: no standalone runtime assets are required by this issue; the installed config resolves exported package dependencies before importing `pyqtgraph_cpp::pyqtgraph_cpp`.

## Commands and results

```sh
cmake --preset release -DCMAKE_INSTALL_PREFIX="$PWD/build/install-P1_02"
```

Exit code: 0

Evidence: configure found Qt/OpenCV dependencies and generated `build/release`.

```text
-- pyqtgraph-cpp Qt 6 Core/Test available: TRUE
-- pyqtgraph-cpp OpenCV 4 available: TRUE
-- Build files have been written to: /home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-106/build/release
```

```sh
cmake --build --preset release --target install --parallel
```

Exit code: 0

Evidence: installed library, targets, package config, and package version config under `build/install-P1_02`.

```text
-- Installing: /home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-106/build/install-P1_02/lib/libpyqtgraph_cpp.a
-- Installing: /home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-106/build/install-P1_02/lib/cmake/pyqtgraph-cpp/pyqtgraph-cppTargets.cmake
-- Installing: /home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-106/build/install-P1_02/lib/cmake/pyqtgraph-cpp/pyqtgraph-cppTargets-release.cmake
-- Installing: /home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-106/build/install-P1_02/lib/cmake/pyqtgraph-cpp/pyqtgraph-cppConfig.cmake
-- Installing: /home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-106/build/install-P1_02/lib/cmake/pyqtgraph-cpp/pyqtgraph-cppConfigVersion.cmake
```

```sh
cmake -S reports/issues/P1.02/consumer -B build/consumer-P1_02 -DCMAKE_PREFIX_PATH="$PWD/build/install-P1_02"
```

Exit code: 0

Evidence: downstream project configured successfully with versioned `find_package(pyqtgraph-cpp 0.1.0 CONFIG REQUIRED)`.

```text
-- Configuring done
-- Generating done
-- Build files have been written to: /home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-106/build/consumer-P1_02
```

```sh
cmake --build build/consumer-P1_02 --parallel
```

Exit code: 0

Evidence: built downstream consumer binary.

```text
[100%] Linking CXX executable p1_02_package_consumer
[100%] Built target p1_02_package_consumer
```

```sh
ctest --test-dir build/consumer-P1_02 --output-on-failure
```

Exit code: 0

Evidence:

```text
1/1 Test #1: P1.02.package-consumer ...........   Passed    0.00 sec

100% tests passed, 0 tests failed out of 1

Label Time Summary:
P1.02    =   0.00 sec*proc (1 test)
```

```sh
scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
```

Exit code: 1

Failure evidence: existing proposed-issue metadata contains blocked-by references that do not match local issues. Representative output:

```text
github-issue-96.md: blocked-by entry does not match a local issue: P0.01
github-issue-106.md: blocked-by entry does not match a local issue: P1.01
github-issue-107.md: blocked-by entry does not match a local issue: P1.01
github-issue-108.md: blocked-by entry does not match a local issue: P0.01
github-issue-109.md: blocked-by entry does not match a local issue: P1.01
github-issue-110.md: blocked-by entry does not match a local issue: P0.07
github-issue-111.md: blocked-by entry does not match a local issue: P0.08
github-issue-212.md: blocked-by entry does not match a local issue: P1.01
github-issue-120.md: blocked-by entry does not match a local issue: P0.06
github-issue-129.md: blocked-by entry does not match a local issue: P0.01
github-issue-130.md: blocked-by entry does not match a local issue: P1.01
github-issue-166.md: blocked-by entry does not match a local issue: P0.01
github-issue-195.md: blocked-by entry does not match a local issue: P0.01
github-issue-197.md: blocked-by entry does not match a local issue: P0.01
github-issue-201.md: blocked-by entry does not match a local issue: P1.01
github-issue-207.md: blocked-by entry does not match a local issue: P0.01
```

```sh
git diff --check
```

Exit code: 0

```sh
git diff --name-only origin/main...HEAD
```

Exit code: 0

Output: no committed branch diff listed at validation time. Current uncommitted worktree changes are listed below.

## Changed-file ownership check

Current uncommitted changed paths:

```text
reports/issues/P1.02/consumer/CMakeLists.txt
reports/issues/P1.02/consumer/main.cpp
reports/issues/P1.02/package-consumer-preimplementation.md
reports/issues/P1.02/package-consumer.md
```

All changed paths are P1.02 focused install/package consumer proof/report artifacts and match the issue-owned install docs/tests scope. No production source, examples, `WORKFLOW.md`, automation policy, or unrelated files were modified.
