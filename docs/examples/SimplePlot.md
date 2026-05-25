# SimplePlot example

## Upstream provenance

- Upstream source: `pyqtgraph/examples/SimplePlot.py`
- PyQtGraph ref: `pyqtgraph-0.14.0`
- Pinned commit: `a20028b98294b9cc8770f2015a92eb342224b788`
- Port issue: `PGEXAMPLE-001`

This is a native Qt/C++ port using the current pyqtgraph-cpp public API. It creates a `pyqtgraph::widgets::PlotWidget`, applies the upstream title text to the widget window title, and adds a `pyqtgraph::graphicsItems::PlotCurveItem` containing 100 deterministic NumPy seed-0 y-values. The y-only `setData` call generates x-values `0..99`.

## Build and run

```sh
cmake --preset dev
cmake --build --preset dev --target pyqtgraph_cpp_examples_simpleplot --parallel
./build/dev/pyqtgraph_cpp_examples_simpleplot
```

Focused smoke test:

```sh
cmake --build --preset dev --target pyqtgraph_cpp_examples_simpleplot_test --parallel
ctest --preset dev -R pyqtgraph_cpp.examples.SimplePlot --output-on-failure
```

## Validation level

`port_manifest.yaml` already lists SimplePlot as a visual-required example. Current validation for this port is limited to a native C++ smoke test plus the repository's placeholder visual gate.

## Current limitations

- `PlotItem` does not currently expose a title API, so the upstream title is applied to `PlotWidget::windowTitle()`.
- `PlotWidget` does not currently expose a plotting convenience method, so the example constructs `PlotCurveItem` directly and parents it to `widget->getPlotItem()`.
- `PlotCurveItem::paint()` is currently a no-op in the owned library code, so this port does not claim native pixel parity yet.
- The existing `scripts/gate visual SimplePlot` path is placeholder-based in this checkout.
