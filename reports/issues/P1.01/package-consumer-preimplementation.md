# P1.01 package consumer pre-implementation proof

Working directory: `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-105`

## Fixture

Added standalone downstream consumer fixture under `reports/issues/P1.01/consumer` that calls `find_package(pyqtgraph-cpp CONFIG REQUIRED)`, links `pyqtgraph_cpp::pyqtgraph_cpp`, and registers test `P1.01.package-consumer`.

## Commands and results before install/export support

```sh
cmake --preset release -DCMAKE_INSTALL_PREFIX="$PWD/build/install-P1_01"
```

Exit code: 0

Evidence: configure completed and generated `build/release`.

```sh
cmake --build --preset release --target install --parallel
```

Exit code: 2

Expected failure evidence:

```text
gmake: *** No rule to make target 'install'.  Stop.
```

```sh
cmake -S reports/issues/P1.01/consumer -B build/consumer-P1_01 -DCMAKE_PREFIX_PATH="$PWD/build/install-P1_01"
```

Exit code: 1

Expected failure evidence:

```text
Could not find a package configuration file provided by "pyqtgraph-cpp"
with any of the following names:

  pyqtgraph-cppConfig.cmake
  pyqtgraph-cpp-config.cmake
```
