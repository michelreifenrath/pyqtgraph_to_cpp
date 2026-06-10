# CppQtGraph

Native C++/Qt port of PyQtGraph-facing APIs. The goal is a C++ library that looks and behaves like PyQtGraph from the outside while staying a native Qt/C++ implementation rather than a Python wrapper.

## Current source of truth

- [`MISSION.md`](MISSION.md) — product goal, hard invariants, and lightweight example-first porting strategy.
- [`examples/example_manifest.yaml`](examples/example_manifest.yaml) — active example-first manifest and pinned reference facts.
- [`AGENTS.md`](AGENTS.md) — local agent/development guidance.

The pinned PyQtGraph reference is `pyqtgraph-0.14.0` at commit `a20028b98294b9cc8770f2015a92eb342224b788`.

## Strategy in short

CppQtGraph does not port PyQtGraph file-by-file. It ports externally visible behavior example-by-example:

1. Read the pinned upstream PyQtGraph example.
2. Freeze deterministic data/fixtures and expected behavior.
3. Build the smallest native C++/Qt example with matching visible output.
4. Add only the missing library behavior required by that example.
5. Validate with smoke tests, numeric oracles, screenshots/visual diffs, and interaction replay as needed.

Python/PyQtGraph is allowed for development oracles and reference screenshots only. It must not be a runtime dependency of the C++ library or examples.

## Status

The repository already contains a useful native C++/Qt 2D plotting foundation. Existing implementation code is intentionally preserved while future work follows the lightweight example-first strategy.

### What works now

| Area | Current coverage |
| --- | --- |
| Build and packaging | Native C++20 target `cppqtgraph`; install/export support for `find_package(CppQtGraph CONFIG REQUIRED)` and imported target `CppQtGraph::CppQtGraph`. |
| Core data/helpers | `ArrayView`, `Point`, `Vector`, `PlotData`, transform helpers, NaN-aware numeric helpers, color helpers, `mkColor`, `glColor`, `mkPen`, `mkBrush`, `ColorMap`, QImage conversion helpers, `SignalProxy`, `ThreadsafeTimer`, and `WidgetGroup` subsets. |
| Graphics scene foundation | `GraphicsScene`, mouse/hover/drag event wrappers, `GraphicsItem`, `GraphicsObject`, `GraphicsWidget`, `GraphicsWidgetAnchor`, `GraphicsLayout`, `GraphicsView`, and `GraphicsLayoutWidget` subsets. |
| Plot/view architecture | `PlotWidget`, `PlotItem`, `ViewBox`, and `AxisItem` with layout, ranges, transforms, autorange, axis ticks/labels/units, log/grid/downsampling/menu state, pan/zoom interaction, and linked-view behavior. |
| Plotting/items/widgets | Focused native slices for line/scatter/image plotting, regions/ROI-like items, legends, grids, labels, color widgets, tree/table/input helper widgets, `ImageView`, and related Qt widgets. |
| Examples | `examples/SimplePlot.cpp` and `examples/ImageItem.cpp` are available as native C++ examples with focused validation. |
| Downstream consumption | A fresh external consumer project can configure, link, build, and run against an installed package using CMake. |

This does **not** mean full PyQtGraph parity. Implemented classes are intentionally partial native C++ subsets. Future work should expand them only when the active example manifest requires the behavior.

## Example-first manifest

Active example work is tracked in:

```text
examples/example_manifest.yaml
```

The manifest records the pinned PyQtGraph reference, current native examples, and the first planned ExampleApp targets. It replaces the old generated all-source inventory workflow.

## Using the library from CMake

### Requirements

- CMake 3.26 or newer.
- A C++20 compiler.
- Qt 6 with `Core`, `Gui`, and `Widgets` for plotting/widgets. Tests also use `Qt6::Test`.
- OpenCV 4 for the default configured baseline. Use `-DCPPQTGRAPH_REQUIRE_OPENCV=OFF` only for intentionally reduced environments.

### Build and install this library

From this repository:

```bash
cmake --preset dev
cmake --build --preset dev --parallel
cmake --install build/dev --prefix /tmp/CppQtGraph-install
```

For a release-style build, use the `release` preset and install from `build/release`.

### Minimal consumer `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.26)
project(cppqtgraph_demo LANGUAGES CXX)

find_package(CppQtGraph CONFIG REQUIRED)

add_executable(cppqtgraph_demo main.cpp)
target_compile_features(cppqtgraph_demo PRIVATE cxx_std_20)
target_link_libraries(cppqtgraph_demo PRIVATE CppQtGraph::CppQtGraph)
```

Configure the consumer with the install prefix on `CMAKE_PREFIX_PATH`:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/tmp/CppQtGraph-install
cmake --build build --parallel
./build/cppqtgraph_demo
```

If you run in a headless environment, use Qt's offscreen platform where appropriate:

```bash
QT_QPA_PLATFORM=offscreen ./build/cppqtgraph_demo
```

### Minimal plotting program

```cpp
#include <cppqtgraph/widgets/PlotWidget.hpp>

#include <QtWidgets/QApplication>

#include <span>
#include <vector>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    cppqtgraph::widgets::PlotWidget plot;
    plot.setWindowTitle(QStringLiteral("CppQtGraph demo"));
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

Useful local commands:

```bash
python3 -m pytest -q
scripts/bootstrap_reference --check --offline
scripts/run_changed_examples --dry-run SimplePlot
cmake --preset dev
cmake --build --preset dev --parallel
ctest --preset dev --output-on-failure
git diff --check
```

`scripts/validate_local` runs the full baseline (pytest, configure, build, ctest, `git diff --check`) as one command; CI runs the same script with `--preset ci-linux`.

Use `.venv/bin/python -m pytest -q` if the system Python does not have pytest installed.

## Lightweight contribution flow

1. Pick one example or directly required behavior slice from `examples/example_manifest.yaml`.
2. Read the pinned upstream PyQtGraph reference.
3. Add or update deterministic fixtures/oracles first when behavior is unclear.
4. Implement only the native C++/Qt behavior needed by that slice.
5. Run focused validation, then the baseline local checks above.
6. Keep generated logs, screenshots, and local runtime artifacts out of the active tree unless they are intentional fixtures or docs.
