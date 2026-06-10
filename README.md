# pyqtgraph_to_cpp

Native C++/Qt port of PyQtGraph-facing APIs. The goal is a C++ library that looks and behaves like PyQtGraph from the outside, while staying a native Qt/C++ implementation rather than a Python wrapper.

## Current source of truth

Use these active documents for current work:

- [`MISSION.md`](MISSION.md) — product goal, scope, non-goals, and hard invariants.
- [`FACTORY_RULES.md`](FACTORY_RULES.md) — issue readiness, scope, evidence, validation, attribution, and protected-file rules.
- [`AGENTS.md`](AGENTS.md) — repository-wide instructions for AI agents.
- [`WORKFLOW.md`](WORKFLOW.md) — product-facing automation policy and validation configuration.
- [`FACTORY_REPO.md`](FACTORY_REPO.md) — notes that the factory control plane lives in a separate repository.

Older planning, long-form workflow docs, and the retired Pi Symphony runtime are archived under `archive/2026-06-01-stale-docs/` for history only.

## Status

The first useful native C++/Qt 2D plotting package is complete. The MVP tracker [#285](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/285) is closed: the library can be built, installed, consumed from CMake, and used for PyQtGraph-like 2D plotting vertical slices.

This does **not** mean full PyQtGraph parity. The manifest is intentionally evidence-backed and still tracks many upstream source files, classes, examples, assets, OpenGL/3D components, exporters, and application subsystems as future work.

### What works now

| Area | Current coverage |
| --- | --- |
| Build and packaging | Native C++20 target `pyqtgraph_cpp`; install/export support for `find_package(pyqtgraph-cpp CONFIG REQUIRED)` and imported target `pyqtgraph_cpp::pyqtgraph_cpp`. |
| Core data/helpers | `ArrayView`, `Point`, `Vector`, `PlotData`, 2D/3D transform helpers, NaN-aware numeric helpers, color helpers, `mkColor`, `glColor`, `mkPen`, `mkBrush`, `ColorMap`, QImage conversion helpers, `SignalProxy`, `ThreadsafeTimer`, and `WidgetGroup` subsets. |
| Graphics scene foundation | `GraphicsScene`, mouse/hover/drag event wrappers, `GraphicsItem`, `GraphicsObject`, `GraphicsWidget`, `GraphicsWidgetAnchor`, `GraphicsLayout`, `GraphicsView`, and `GraphicsLayoutWidget` subsets. |
| Plot/view architecture | `PlotWidget`, `PlotItem`, `ViewBox`, and `AxisItem` with layout, ranges, transforms, autorange, axis ticks/labels/units, log/grid/downsampling/menu state, pan/zoom interaction, and linked-view behavior. |
| Plotting items | `plot(y)` / `plot(x, y)` line plotting through `PlotDataItem` and `PlotCurveItem`; plus validated slices for scatter, graph/curve-point, bar/error/fill-between, boxplot/pcolor mesh, image/non-uniform image, histogram/LUT/color-bar, isocurve, ROI, infinite/linear regions, target, legend, grid, v-ticks, annotations, scale bar, and gradient editor/legend items. |
| Widgets | The current widget manifest coverage is closed (`widgets: 33/33 complete` in `scripts/summarize_status`). Covered slices include plotting/layout widgets, histogram/LUT and raw image widgets, color widgets, tree/table widgets, input widgets, helper/layout widgets, `ImageView`, and `ScatterPlotWidget`. |
| Examples | `examples/SimplePlot.cpp` and `examples/ImageItem.cpp` are available as native C++ examples with focused validation. |
| Downstream consumption | A fresh external consumer project can configure, link, build, and run against an installed package using CMake. |

Coverage is implemented as PyQtGraph-like, issue-scoped C++/Qt behavior. Many classes are partial subsets rather than complete copies of every upstream Python method.

### What is not complete yet

| Area | Current status |
| --- | --- |
| Full PyQtGraph API parity | Not complete. `port_manifest.yaml` remains the broader source/example/class backlog. |
| ParameterTree, DockArea, Flowchart | Post-MVP work; open P6 issues track these subsystems. |
| OpenGL / 3D | Post-MVP work; open P7 issues track GL backend policy, GL items, rendering, and coverage. |
| Exporters | Post-MVP work; open P8 issues track exporter registry and file exporters. |
| Broad example/assets parity | Only focused example slices are available. P9 issues track assets, dependency matrix, example ports, and final example-suite rollups. |
| Full final/release evidence bundles | P10 rollup/performance/final-acceptance work remains open. |
| Python-only behavior | Python wrappers/import machinery, REPL/Jupyter behavior, Python-only debugging helpers, multiprocessing/remote graphics internals, and Python-only console behavior are intentional non-goals unless an equivalence issue says otherwise. |
| Performance parity | Broad performance tuning and benchmark parity are deferred until more of the 2D API surface is stable. |

The pinned PyQtGraph reference is `pyqtgraph-0.14.0` at commit `a20028b98294b9cc8770f2015a92eb342224b788`.

## Using the library from CMake

### Requirements

- CMake 3.26 or newer.
- A C++20 compiler.
- Qt 6 with `Core`, `Gui`, and `Widgets` for plotting/widgets. Tests also use `Qt6::Test`.
- OpenCV 4 for the default configured baseline. Use `-DPYQTGRAPH_CPP_REQUIRE_OPENCV=OFF` only for intentionally reduced environments.

### Build and install this library

From this repository:

```bash
cmake --preset dev
cmake --build --preset dev --parallel
cmake --install build/dev --prefix /tmp/pyqtgraph-cpp-install
```

For a release-style build, use the `release` preset and install from `build/release`.

### Minimal consumer `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.26)
project(pgcpp_demo LANGUAGES CXX)

find_package(pyqtgraph-cpp CONFIG REQUIRED)

add_executable(pgcpp_demo main.cpp)
target_compile_features(pgcpp_demo PRIVATE cxx_std_20)
target_link_libraries(pgcpp_demo PRIVATE pyqtgraph_cpp::pyqtgraph_cpp)
```

Configure the consumer with the install prefix on `CMAKE_PREFIX_PATH`:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/tmp/pyqtgraph-cpp-install
cmake --build build --parallel
./build/pgcpp_demo
```

If you run in a headless environment, use Qt's offscreen platform where appropriate:

```bash
QT_QPA_PLATFORM=offscreen ./build/pgcpp_demo
```

### Minimal plotting program

```cpp
#include <pyqtgraph/widgets/PlotWidget.hpp>

#include <QtWidgets/QApplication>

#include <span>
#include <vector>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    pyqtgraph::widgets::PlotWidget plot;
    plot.setWindowTitle(QStringLiteral("pyqtgraph-cpp demo"));
    plot.setTitle(QStringLiteral("Native C++/Qt plot"));
    plot.setLabel(QStringLiteral("bottom"), QStringLiteral("sample"));
    plot.setLabel(QStringLiteral("left"), QStringLiteral("value"));
    plot.addLegend();

    std::vector<double> x{0.0, 1.0, 2.0, 3.0, 4.0};
    std::vector<double> y{0.0, 1.0, 0.25, 1.5, 1.0};
    plot.plot(std::span<const double>(x.data(), x.size()),
              std::span<const double>(y.data(), y.size()),
              QStringLiteral("signal"));

    plot.resize(800, 600);
    plot.show();
    return QApplication::exec();
}
```

You can also inspect the in-repository examples:

- [`examples/SimplePlot.cpp`](examples/SimplePlot.cpp)
- [`examples/ImageItem.cpp`](examples/ImageItem.cpp)

## Development and validation commands

Useful product-repo commands:

```bash
python3 -m pytest -q
cmake --preset dev
cmake --build --preset dev --parallel
ctest --preset dev --output-on-failure
scripts/summarize_status
scripts/generate_manifest --check
scripts/check_manifest_ownership --manifest port_manifest.yaml --ownership ownership.yaml
scripts/check_changed_file_ownership --base origin/main
scripts/run_changed_examples --help
scripts/gate commit
```

Use `.venv/bin/python -m pytest -q` if the system Python does not have pytest installed.

The Dark Factory dispatcher, merge controller, and factory-only scripts are not vendored here; see [`FACTORY_REPO.md`](FACTORY_REPO.md).

## Operational flow

1. Create or update one fine-grained GitHub issue with clear scope, owned files/selectors, TDD plan, validation commands, and acceptance criteria.
2. Add `ai:ready` only when dependencies are resolved and readiness gates pass.
3. Automation claims the issue, creates an isolated worktree, and runs the configured workers.
4. Review and release gates produce an evidence-backed PR.
5. The validation/merge controller performs holdout validation and either merges, schedules focused rework, or marks `human-review`.

Do not push to `main`. Implementation, rework, review, and release workers must not merge.
