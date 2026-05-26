# P1.03 package consumer pre-implementation proof

Working directory: `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-107`

## Fixture

Added standalone downstream consumer fixture under `reports/issues/P1.03/consumer` that calls only `find_package(pyqtgraph-cpp CONFIG REQUIRED)`, asserts that the package config replayed OpenCV discovery, links `pyqtgraph_cpp::pyqtgraph_cpp` plus the replayed `${OpenCV_LIBS}`, and registers test `P1.03.package-consumer`.

## Commands and results before OpenCV package-config replay

```sh
rm -rf build/install-P1_03 build/consumer-P1_03
```

Exit code: 0

```sh
cmake --preset release -DCMAKE_INSTALL_PREFIX="$PWD/build/install-P1_03"
```

Exit code: 0

Evidence: configure completed and generated `build/release` with `pyqtgraph-cpp OpenCV 4 available: TRUE`.

```sh
cmake --build --preset release --target install --parallel
```

Exit code: 0

Evidence: installed `lib/libpyqtgraph_cpp.a`, public headers, `pyqtgraph-cppConfig.cmake`, `pyqtgraph-cppConfigVersion.cmake`, and exported targets under `build/install-P1_03`.

```sh
cmake -S reports/issues/P1.03/consumer -B build/consumer-P1_03 -DCMAKE_PREFIX_PATH="$PWD/build/install-P1_03"
```

Exit code: 1

Expected failure evidence:

```text
CMake Error at CMakeLists.txt:8 (message):
  pyqtgraph-cpp package config did not replay its OpenCV dependency

-- Configuring incomplete, errors occurred!
```
