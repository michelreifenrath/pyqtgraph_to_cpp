# P1.02 package consumer pre-implementation proof

## Red observation 1: missing fixture (parent-provided)

Command:

```sh
cmake -S reports/issues/P1.02/consumer -B build/consumer-P1_02 -DCMAKE_PREFIX_PATH="$PWD/build/install-P1_02"
```

Exit code: 1

Evidence: CMake failed before this fixture existed because the source directory `reports/issues/P1.02/consumer` did not exist.

## Red observation 2: clean prefix has no installed package config

Command:

```sh
rm -rf build/install-P1_02 build/consumer-P1_02
cmake -S reports/issues/P1.02/consumer -B build/consumer-P1_02 -DCMAKE_PREFIX_PATH="$PWD/build/install-P1_02"
```

Exit code: 1

Concise evidence:

```text
CMake Error at CMakeLists.txt:5 (find_package):
  Could not find a package configuration file provided by "pyqtgraph-cpp"
  (requested version 0.1.0) with any of the following names:

    pyqtgraph-cppConfig.cmake
    pyqtgraph-cpp-config.cmake
```
