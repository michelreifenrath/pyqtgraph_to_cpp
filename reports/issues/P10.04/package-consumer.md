# P10.04 package-consumer validation

## Scope and changed files

Issue-owned P10.04 package-consumer proof files changed:

- `reports/issues/P10.04/consumer/CMakeLists.txt`
- `reports/issues/P10.04/consumer/main.cpp`
- `reports/issues/P10.04/package-consumer-preimplementation.md`
- `reports/issues/P10.04/package-consumer.md`

Shared wiring paths changed: none.

Manifest/dashboard update: not applicable; this issue adds only a package-consumer proof fixture/report and does not change tracked source classes, examples, or assets.

## PyQtGraph reference

Primary reference was `pyqtgraph-0.14.0` at pinned commit `a20028b98294b9cc8770f2015a92eb342224b788`.

Concrete upstream references used for the downstream SimplePlot-style proof:

- `pyqtgraph/examples/SimplePlot.py` creates the simplest plot with `pg.plot(...)` and runs the Qt app for standalone execution.
- `pyqtgraph/__init__.py` `plot()` creates a `PlotWidget`, forwards data through `w.plot(...)`, shows the widget, and returns it.
- `pyqtgraph/widgets/PlotWidget.py` defines `PlotWidget` as a graphics view containing one `PlotItem`, and `getPlotItem()` returns that item.
- `pyqtgraph/graphicsItems/PlotItem/PlotItem.py` `plot()` creates/adds a plotted data item and returns it.
- `tests/graphicsItems/test_PlotCurveItem.py` exercises adding a `PlotCurveItem` to a view, auto-ranging, and rendering connected curve data.

Optional Qt/OpenCV/NumPy internals were not fetched; public PyQtGraph behavior and existing native package plumbing were sufficient.

## Fixture behavior

`reports/issues/P10.04/consumer` is a fresh downstream CMake project. It:

- calls `find_package(pyqtgraph-cpp CONFIG REQUIRED)`;
- asserts imported target `pyqtgraph_cpp::pyqtgraph_cpp` exists;
- links only the installed package target;
- builds `p10_04_package_consumer`;
- registers `P10.04.package-consumer` with `QT_QPA_PLATFORM=offscreen`;
- creates `pyqtgraph::widgets::PlotWidget`, obtains its `PlotItem`, calls `PlotItem::plot(x, y, name)`, verifies returned `PlotCurveItem` data, shows the widget, processes events, and verifies an offscreen grab produces a pixmap.

Runtime assets/dependencies: no standalone runtime assets are required for this fixture. The installed config resolves Qt/OpenCV dependencies before importing `pyqtgraph_cpp::pyqtgraph_cpp`, and the CTest environment selects offscreen Qt.

## Pre-implementation failing proof

See `reports/issues/P10.04/package-consumer-preimplementation.md`.

Command:

```bash
rm -rf build/install-P10_04 build/consumer-P10_04
cmake -S reports/issues/P10.04/consumer -B build/consumer-P10_04 -DCMAKE_PREFIX_PATH="$PWD/build/install-P10_04"
```

Exit code: 1

Evidence log: `/home/michel/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp_factory/artifacts/runs/dc2b1ab96f3e268e3641effd19ee2652/p10_04_pre_configure.log`

Failure was the expected missing clean-prefix package config (`pyqtgraph-cppConfig.cmake`).

## Validation commands after implementation

All commands were run from the target repository `/home/michel/.archon/workspaces/code/pyqtgraph_to_cpp_factory/worktrees/archon/task-factory-test-issue-212-mvp-1780725721/target-repo`.

| Command | Exit | Evidence |
| --- | ---: | --- |
| `cmake --preset release -DCMAKE_INSTALL_PREFIX="$PWD/build/install-P10_04"` | 0 | `/home/michel/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp_factory/artifacts/runs/dc2b1ab96f3e268e3641effd19ee2652/p10_04_01_configure_release.log` |
| `cmake --build --preset release --target install --parallel` | 0 | `/home/michel/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp_factory/artifacts/runs/dc2b1ab96f3e268e3641effd19ee2652/p10_04_02_install.log` |
| `cmake -S reports/issues/P10.04/consumer -B build/consumer-P10_04 -DCMAKE_PREFIX_PATH="$PWD/build/install-P10_04"` | 0 | `/home/michel/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp_factory/artifacts/runs/dc2b1ab96f3e268e3641effd19ee2652/p10_04_03_consumer_configure.log` |
| `cmake --build build/consumer-P10_04 --parallel` | 0 | `/home/michel/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp_factory/artifacts/runs/dc2b1ab96f3e268e3641effd19ee2652/p10_04_04_consumer_build.log` |
| `ctest --test-dir build/consumer-P10_04 --output-on-failure` | 0 | `/home/michel/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp_factory/artifacts/runs/dc2b1ab96f3e268e3641effd19ee2652/p10_04_05_consumer_ctest.log` |
| `scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp` | 127 | `/home/michel/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp_factory/artifacts/runs/dc2b1ab96f3e268e3641effd19ee2652/p10_04_06_check_proposed_issues.log` |
| `"$FACTORY_REPO_DIR/scripts/check_proposed_issues" --source github --repo michelreifenrath/pyqtgraph_to_cpp` | 0 | `/home/michel/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp_factory/artifacts/runs/dc2b1ab96f3e268e3641effd19ee2652/p10_04_06b_factory_check_proposed_issues.log` |
| `git diff --check` | 0 | `/home/michel/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp_factory/artifacts/runs/dc2b1ab96f3e268e3641effd19ee2652/p10_04_07_git_diff_check.log` |
| `git diff --name-only origin/main...HEAD` | 0 | `/home/michel/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp_factory/artifacts/runs/dc2b1ab96f3e268e3641effd19ee2652/p10_04_10_git_diff_name_only_origin.log` |
| changed-file list generation | 0 | `/home/michel/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp_factory/artifacts/runs/dc2b1ab96f3e268e3641effd19ee2652/changed-files.txt` |
| `python3 "$FACTORY_REPO_DIR/scripts/factory/check_pr_scope.py" --issue-file "$ARTIFACTS_DIR/issue.json" --changed-files-file "$ARTIFACTS_DIR/changed-files.txt"` | 0 | `/home/michel/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp_factory/artifacts/runs/dc2b1ab96f3e268e3641effd19ee2652/p10_04_08_check_pr_scope.log` |
| `PYTHONPATH="$FACTORY_REPO_DIR" python3 "$FACTORY_REPO_DIR/scripts/gate" commit --dry-run --workflow "$TARGET_REPO_DIR/WORKFLOW.md"` | 0 | `/home/michel/.archon/workspaces/michelreifenrath/pyqtgraph_to_cpp_factory/artifacts/runs/dc2b1ab96f3e268e3641effd19ee2652/p10_04_09_gate_commit_dry_run.log` |

`ctest` result:

```text
1/1 Test #1: P10.04.package-consumer ..........   Passed
100% tests passed, 0 tests failed out of 1
```

The product-relative proposed-issues script command could not be completed because this checkout has no `scripts/check_proposed_issues` executable (shell exit 127). The available product `scripts/` entries are `bootstrap_reference`, `check_attribution`, `check_visual_artifacts`, `claim_ticket`, `doctor_local`, `gate`, `generate_manifest`, `run_changed_examples`, `run_performance`, and `summarize_status`. The equivalent Factory helper was run by absolute path and exited 0.
