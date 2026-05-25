# P1.01 package consumer proof

Working directory: `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-105`

## Scope and ownership

- Manifest-expanded target paths: not applicable; this issue has no manifest source or example selectors.
- Shared build/package wiring changed: `CMakeLists.txt`; `cmake/pyqtgraph-cppConfig.cmake.in`.
- Focused P1.01 proof/report artifacts changed: `reports/issues/P1.01/consumer/CMakeLists.txt`; `reports/issues/P1.01/consumer/main.cpp`; `reports/issues/P1.01/package-consumer-preimplementation.md`; `reports/issues/P1.01/package-consumer.md`.
- Manifest/dashboard update: not applicable; no tracked source, class, example, or asset status changed.

## Installed package artifacts

- Install prefix: `build/install-P1_01`
- Library: `build/install-P1_01/lib/libpyqtgraph_cpp.a`
- Package config: `build/install-P1_01/lib/cmake/pyqtgraph-cpp/pyqtgraph-cppConfig.cmake`
- Package version config: `build/install-P1_01/lib/cmake/pyqtgraph-cpp/pyqtgraph-cppConfigVersion.cmake`
- Exported targets: `build/install-P1_01/lib/cmake/pyqtgraph-cpp/pyqtgraph-cppTargets.cmake`
- Exported consumer target: `pyqtgraph_cpp::pyqtgraph_cpp`
- Exported public dependencies: Qt 6 Core/Gui/Widgets and `pyqtgraph_cpp::pyqtgraph_cpp_project_options`; build-only warnings target is not exported.
- Downstream consumer binary: `build/consumer-P1_01/p1_01_package_consumer`

## Commands and results

```sh
rm -rf build/install-P1_01 build/consumer-P1_01
```

Exit code: 0

```sh
cmake --preset release -DCMAKE_INSTALL_PREFIX="$PWD/build/install-P1_01"
```

Exit code: 0

Evidence: configure completed and generated `build/release`.

```sh
cmake --build --preset release --target install --parallel
```

Exit code: 0

Evidence: installed `lib/libpyqtgraph_cpp.a`, public headers, `pyqtgraph-cppConfig.cmake`, `pyqtgraph-cppConfigVersion.cmake`, and `pyqtgraph-cppTargets.cmake` under `build/install-P1_01`.

```sh
cmake -S reports/issues/P1.01/consumer -B build/consumer-P1_01 -DCMAKE_PREFIX_PATH="$PWD/build/install-P1_01"
```

Exit code: 0

Evidence: downstream project configured successfully with `find_package(pyqtgraph-cpp CONFIG REQUIRED)`.

```sh
cmake --build build/consumer-P1_01 --parallel
```

Exit code: 0

Evidence: built `build/consumer-P1_01/p1_01_package_consumer`.

```sh
ctest --test-dir build/consumer-P1_01 --output-on-failure
```

Exit code: 0

Evidence:

```text
1/1 Test #1: P1.01.package-consumer ...........   Passed    0.00 sec

100% tests passed, 0 tests failed out of 1

Label Time Summary:
P1.01    =   0.00 sec*proc (1 test)
```

```sh
scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
```

Exit code: 1

Failure evidence:

```text
github-issue-96.md: blocked-by entry does not match a local issue: P0.01
github-issue-100.md: blocked-by entry does not match a local issue: P0.01
github-issue-101.md: blocked-by entry does not match a local issue: P0.01
github-issue-102.md: blocked-by entry does not match a local issue: P0.01
github-issue-105.md: blocked-by entry does not match a local issue: P0.01
github-issue-108.md: blocked-by entry does not match a local issue: P0.01
github-issue-129.md: blocked-by entry does not match a local issue: P0.01
github-issue-166.md: blocked-by entry does not match a local issue: P0.01
github-issue-195.md: blocked-by entry does not match a local issue: P0.01
github-issue-197.md: blocked-by entry does not match a local issue: P0.01
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

Output:

```text
CMakeLists.txt
cmake/pyqtgraph-cppConfig.cmake.in
reports/issues/P1.01/consumer/CMakeLists.txt
reports/issues/P1.01/consumer/main.cpp
reports/issues/P1.01/package-consumer-preimplementation.md
reports/issues/P1.01/package-consumer.md
```
