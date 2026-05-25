# PGEXAMPLE-001 SimplePlot example

## Summary

- Added a native C++ `examples/SimplePlot.cpp` port of upstream `pyqtgraph/examples/SimplePlot.py` using the current public PlotWidget/PlotCurveItem API only.
- Added a reusable `pyqtgraph::examples::createSimplePlotExample()` factory/test hook and guarded `main()` with `PYQTGRAPH_CPP_SIMPLEPLOT_NO_MAIN`.
- Added a focused C++ smoke test and CMake registration for the example executable and test.
- Added example documentation with provenance, run instructions, validation level, and current limitations.

## Changed files

- `CMakeLists.txt`
- `examples/SimplePlot.cpp`
- `tests/examples/test_SimplePlot.cpp`
- `docs/examples/SimplePlot.md`
- `reports/agents/PGEXAMPLE-001.md`

`port_manifest.yaml` was not changed; the existing SimplePlot inventory and validation metadata were sufficient.

## TDD evidence

### Red

After adding `tests/examples/test_SimplePlot.cpp` and CMake registration, before implementing `examples/SimplePlot.cpp`:

```sh
cmake --preset dev && cmake --build --preset dev --target pyqtgraph_cpp_examples_simpleplot pyqtgraph_cpp_examples_simpleplot_test --parallel
```

Exit code: 1

Observed failure:

```text
CMake Error at CMakeLists.txt:381 (add_executable):
  Cannot find source file:

    examples/SimplePlot.cpp

CMake Error at CMakeLists.txt:381 (add_executable):
  No SOURCES given to target: pyqtgraph_cpp_examples_simpleplot
```

### Green

After implementing `examples/SimplePlot.cpp`:

```sh
cmake --preset dev && cmake --build --preset dev --target pyqtgraph_cpp_examples_simpleplot pyqtgraph_cpp_examples_simpleplot_test --parallel
```

Exit code: 0

## Validation commands

- `cmake --preset dev` — exit code 0.
- `cmake --build --preset dev --target pyqtgraph_cpp_examples_simpleplot pyqtgraph_cpp_examples_simpleplot_test --parallel` — exit code 0.
- `ctest --preset dev -R pyqtgraph_cpp.examples.SimplePlot --output-on-failure` — exit code 0; 1/1 focused tests passed.
- `scripts/gate visual SimplePlot` — exit code 0; ran the placeholder visual path for `tests/examples/test_SimplePlot_visual.py`.
- `git diff --check` — exit code 0.
- `scripts/gate commit` — exit code 0; ran diff checks and `python3 -m pytest -q`.
- `python3 -m pytest -q` — exit code 0; 259 passed.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — exit code 0; workflow valid.

## Known limitations

- `PlotItem` has no current title API, so the upstream title text is applied to the `PlotWidget` window title.
- `PlotWidget` has no current plotting convenience API, so the example creates a `PlotCurveItem` directly and parents it to `widget->getPlotItem()`.
- `PlotCurveItem::paint()` is currently a no-op outside this issue's owned files; no native pixel parity is claimed.
- The visual gate in this checkout is placeholder-based, so it is only smoke-level evidence.

## Unavailable or incompatible issue-requested commands

- `scripts/gate focus PGEXAMPLE-001` — exit code 2; `focus mode does not accept an example name`.
- `scripts/run_changed_examples` — exit code 127; script is absent in this checkout.
