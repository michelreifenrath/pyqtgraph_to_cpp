# pyqtgraph_to_cpp

Repository-owned factory automation and source tree for translating PyQtGraph into a native C++ library.

## Current source of truth

Use these active documents for current work:

- [`MISSION.md`](MISSION.md) — product goal, scope, non-goals, and hard invariants.
- [`FACTORY_RULES.md`](FACTORY_RULES.md) — issue readiness, scope, evidence, validation, auto-merge, attribution, and protected-file rules.
- [`AGENTS.md`](AGENTS.md) — repository-wide instructions for AI agents.
- [`WORKFLOW.md`](WORKFLOW.md) — machine-readable factory runtime configuration.

Older planning, long-form workflow docs, and the retired Pi Symphony runtime are archived under `archive/2026-06-01-stale-docs/` for history only.

## MVP status: first useful native C++/Qt 2D plotting slice

Status: the implementation checklist tracked in [issue #285](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/285) is complete. This is an installable native C++/Qt library with a PyQtGraph-like 2D plotting vertical slice, not full PyQtGraph parity.

### Implemented PyQtGraph-like functionality

| Area | Current MVP coverage |
| --- | --- |
| Build and packaging | Native C++20 target `pyqtgraph_cpp`; install/export support for `find_package(pyqtgraph-cpp CONFIG REQUIRED)` and imported target `pyqtgraph_cpp::pyqtgraph_cpp`. |
| Core data/helpers | `Point`, `Vector`, `PlotData`, NaN-aware numeric helpers, color helpers, `mkColor`, `Color`, `glColor`, `mkPen`, `mkBrush`, `ColorMap`, QImage conversion helpers, `SignalProxy`, and `ThreadsafeTimer` subsets. |
| Graphics scene foundation | `GraphicsScene`, mouse/hover/drag event wrappers, `GraphicsItem`, `GraphicsObject`, `GraphicsWidget`, and `GraphicsWidgetAnchor` subsets. |
| View and axes | `PlotWidget`, `PlotItem`, `ViewBox`, and `AxisItem` with basic layout, ranges, transforms, autorange, axis ticks/labels/units, pan/zoom interaction, and linked-view behavior. |
| Plotting path | `plot(x, y)` / `plot(y)`-style line plotting through `PlotItem`, `PlotDataItem`, and `PlotCurveItem`, including curve painting, pens, data updates, empty/NaN data handling, and ViewBox transform synchronization. |
| Legend/title/layout slice | Basic `PlotItem` title, axis visibility, legend creation, legend entries, and layout integration. |
| Example and validation | `examples/SimplePlot.cpp` is ported as the first real example and has deterministic C++ tests plus visual comparison evidence against the pinned PyQtGraph reference. |
| Downstream consumption | A fresh external consumer project can configure, link, build, and run against the installed package via CMake. |

### Not yet ported / outside the MVP

| Area | Current status |
| --- | --- |
| Full PyQtGraph API parity | Not complete. The manifest still contains many unported upstream files/classes beyond the MVP slice. |
| Additional plot items | `ScatterPlotItem`, bar/error/fill-between items, `ImageItem`, histogram/LUT/color-bar items, `LinearRegionItem`, `InfiniteLine`, `TargetItem`, grid/vtick helpers, and similar 2D expansion items are post-MVP work. |
| Image and analysis workflows | `ImageView`, broader image-processing workflows, and full LUT/histogram tooling are not yet available. |
| Application subsystems | Console, DockArea, Flowchart, ParameterTree, exporters, broad widget coverage, and multiprocess/remote graphics internals are not ported. |
| OpenGL / 3D | OpenGL and 3D plotting remain out of scope until stable 2D parity is further along. |
| Examples | Only the SimplePlot vertical slice is ported; most PyQtGraph examples are still not implemented as C++ examples. |
| Python-only behavior | Python wrappers/import machinery, REPL/Jupyter behavior, Python-only debugging helpers, and Python-only internals are intentional non-goals for this native C++ library. |
| Performance baseline | Broad performance tuning and benchmark parity are deferred until the 2D API surface is more complete. |

For the broader upstream source-file and example backlog inventory, see [`port_manifest.yaml`](port_manifest.yaml). The pinned PyQtGraph reference is `pyqtgraph-0.14.0` at commit `a20028b98294b9cc8770f2015a92eb342224b788`.

## Automation model

This repo contains an issue-to-PR factory loop:

- GitHub Issues are the source of truth.
- Label an issue `ai:ready` only after readiness gates pass.
- Archon workflows and factory scripts own the active automation path.
- Pi workers may run inside isolated git worktrees, but implementation, rework, review, and release workers never merge.
- The validation/merge controller may auto-merge only when `WORKFLOW.md` enables `policy.auto_merge` and all governed gates in `FACTORY_RULES.md` pass.
- Worktrees live under `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp`.

## Common commands

```bash
scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
scripts/factory/check_issue_ready.py --issue-file <issue.json>
scripts/factory/check_pr_scope.py --issue-file <issue.json> --changed-files-file <paths.txt>
scripts/gate focus
scripts/gate commit
scripts/run_autoreview --mode branch
python3 -m pytest -q
```

Use `.venv/bin/python -m pytest -q` when the system Python does not have pytest installed.

## Operational flow

1. Create or update one fine-grained GitHub issue with clear scope, owned files/selectors, TDD plan, validation commands, and acceptance criteria.
2. Add `ai:ready` only when dependencies are resolved and readiness gates pass.
3. Automation claims the issue, creates an isolated worktree, and runs the configured workers.
4. Review and release gates produce an evidence-backed PR.
5. The validation/merge controller performs holdout validation and either merges, schedules focused rework, or marks `human-review`.
