# P10.04 pre-implementation package-consumer proof

Added a standalone downstream consumer fixture under `reports/issues/P10.04/consumer` that calls `find_package(pyqtgraph-cpp CONFIG REQUIRED)`, asserts the exported target `pyqtgraph_cpp::pyqtgraph_cpp`, links it, exercises the installed `PlotWidget` / `PlotItem::plot` / `PlotCurveItem` API, and registers `P10.04.package-consumer` with offscreen Qt.

## Expected failing proof

Before installing the package to the clean P10.04 prefix, the downstream configure command fails at package discovery:

```bash
rm -rf build/install-P10_04 build/consumer-P10_04
cmake -S reports/issues/P10.04/consumer -B build/consumer-P10_04 -DCMAKE_PREFIX_PATH="$PWD/build/install-P10_04"
```

Exit code: 1

Evidence log: `/home/michel/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp_factory/artifacts/runs/dc2b1ab96f3e268e3641effd19ee2652/p10_04_pre_configure.log`

Relevant failure:

```text
Could not find a package configuration file provided by "pyqtgraph-cpp"
with any of the following names:

  pyqtgraph-cppConfig.cmake
  pyqtgraph-cpp-config.cmake
```
