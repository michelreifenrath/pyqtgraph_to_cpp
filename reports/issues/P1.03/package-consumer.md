# P1.03 package consumer proof

Working directory: `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-107`

## Scope and ownership

- Manifest-expanded target paths: not applicable; this issue has no manifest source or example selectors.
- Shared build/package wiring changed: `CMakeLists.txt`; `cmake/pyqtgraph-cppConfig.cmake.in`.
- Focused P1.03 proof/report artifacts changed: `reports/issues/P1.03/consumer/CMakeLists.txt`; `reports/issues/P1.03/consumer/main.cpp`; `reports/issues/P1.03/package-consumer-preimplementation.md`; `reports/issues/P1.03/package-consumer.md`.
- Manifest/dashboard update: not applicable; no tracked source, class, example, or asset status changed.

## Installed package artifacts

- Install prefix: `build/install-P1_03`
- Library: `build/install-P1_03/lib/libpyqtgraph_cpp.a`
- Package config: `build/install-P1_03/lib/cmake/pyqtgraph-cpp/pyqtgraph-cppConfig.cmake`
- Package version config: `build/install-P1_03/lib/cmake/pyqtgraph-cpp/pyqtgraph-cppConfigVersion.cmake`
- Exported targets: `build/install-P1_03/lib/cmake/pyqtgraph-cpp/pyqtgraph-cppTargets.cmake`
- Exported consumer target: `pyqtgraph_cpp::pyqtgraph_cpp`
- Exported public dependencies replayed by package config: Qt 6 Core/Gui/Widgets and OpenCV 4.
- OpenGL replay: not added. Targeted evidence found no current public OpenGL dependency in `include/`, `src/`, `CMakeLists.txt`, or `cmake/`; only future manifest references mention `QOpenGLWidget`.
- Downstream consumer binary: `build/consumer-P1_03/p1_03_package_consumer`

## Dependency replay behavior

The downstream fixture calls only:

```cmake
find_package(pyqtgraph-cpp CONFIG REQUIRED)
```

It does not call `find_package(OpenCV)`. After package discovery it asserts `OpenCV_FOUND`, `OpenCV_INCLUDE_DIRS`, and `OpenCV_LIBS`, then links with `pyqtgraph_cpp::pyqtgraph_cpp` and `${OpenCV_LIBS}` and includes `${OpenCV_INCLUDE_DIRS}`. The executable uses Qt-dependent pyqtgraph-cpp API (`Point`, `Vector`, `mkColor`) and OpenCV API (`cv::Mat`, `cv::Scalar`).

Generated package-config evidence:

```text
find_dependency(Qt6 6 COMPONENTS ${_pyqtgraph_cpp_qt_components})
find_dependency(OpenCV 4)
```

## Commands and results

```sh
rm -rf build/install-P1_03 build/consumer-P1_03
```

Exit code: 0

```sh
cmake --preset release -DCMAKE_INSTALL_PREFIX="$PWD/build/install-P1_03"
```

Exit code: 0

Evidence: configure completed and generated `build/release`; output included `pyqtgraph-cpp OpenCV 4 available: TRUE`.

```sh
cmake --build --preset release --target install --parallel
```

Exit code: 0

Evidence: installed `lib/libpyqtgraph_cpp.a`, public headers, `pyqtgraph-cppConfig.cmake`, `pyqtgraph-cppConfigVersion.cmake`, and `pyqtgraph-cppTargets.cmake` under `build/install-P1_03`.

```sh
cmake -S reports/issues/P1.03/consumer -B build/consumer-P1_03 -DCMAKE_PREFIX_PATH="$PWD/build/install-P1_03"
```

Exit code: 0

Evidence: downstream project configured successfully through only `find_package(pyqtgraph-cpp CONFIG REQUIRED)`; output included `Found OpenCV: /usr (found suitable version "4.6.0", minimum required is "4")`.

```sh
cmake --build build/consumer-P1_03 --parallel
```

Exit code: 0

Evidence: built `build/consumer-P1_03/p1_03_package_consumer`.

```sh
ctest --test-dir build/consumer-P1_03 --output-on-failure
```

Exit code: 0

Evidence:

```text
1/1 Test #1: P1.03.package-consumer ...........   Passed    0.01 sec

100% tests passed, 0 tests failed out of 1

Label Time Summary:
P1.03    =   0.01 sec*proc (1 test)
```

```sh
scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
```

Exit code: 1

Failure evidence from existing proposed-issue metadata outside P1.03 scope:

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

Output: empty; this worktree has uncommitted P1.03 changes, but no committed branch delta beyond `HEAD` for this command to list.
