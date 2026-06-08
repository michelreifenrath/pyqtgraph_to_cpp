# P4.15 completion report

Issue: GitHub [#157](https://github.com/michelreifenrath/pyqtgraph_to_cpp/issues/157)

## Summary

Closed graphicsItems manifest/dashboard coverage to **47/47** target-file presence by adding the remaining manifest-owned translation units and focused `P4_15` manifest-infra pytest coverage.

## TDD red run

Command run before implementation:

```bash
python3 -m pytest -q tests -k P4_15
```

Exit code: `1`

Failure: `test_P4_15_graphicsitems_manifest_dashboard_reports_complete_coverage` observed generated manifest graphicsItems coverage at **33/47** `target_presence=all`.

## Final validation

| Command | Exit code | Result |
| --- | ---: | --- |
| `python3 -m pytest -q tests -k P4_15` | 0 | `5 passed, 268 deselected in 23.08s` |
| `python3 scripts/generate_manifest --check` | 0 | `port manifest verified (213 source files, 129 examples, 355 classes)` |
| `python3 -m pytest -q` | 0 | `262 passed, 11 skipped in 47.16s` |
| `git diff --check` | 0 | no whitespace errors |

## Dashboard outcome

Generated manifest graphicsItems selector:

`graphicsItems: 47/47 target files present`

All 47 graphicsItems `source_files` rows now report `target_presence: all` and `status: ported`.

## Manifest-expanded target paths added

- `include/pyqtgraph/graphicsItems/GradientPresets.hpp`, `src/pyqtgraph/graphicsItems/GradientPresets.cpp`
- `include/pyqtgraph/graphicsItems/ItemGroup.hpp`, `src/pyqtgraph/graphicsItems/ItemGroup.cpp`
- `include/pyqtgraph/graphicsItems/UIGraphicsItem.hpp`, `src/pyqtgraph/graphicsItems/UIGraphicsItem.cpp`
- `include/pyqtgraph/graphicsItems/__init__.hpp`, `src/pyqtgraph/graphicsItems/__init__.cpp`
- `include/pyqtgraph/graphicsItems/PlotItem/__init__.hpp`, `src/pyqtgraph/graphicsItems/PlotItem/__init__.cpp`
- `include/pyqtgraph/graphicsItems/ViewBox/__init__.hpp`, `src/pyqtgraph/graphicsItems/ViewBox/__init__.cpp`
- `include/pyqtgraph/graphicsItems/ViewBox/ViewBoxMenu.hpp`, `src/pyqtgraph/graphicsItems/ViewBox/ViewBoxMenu.cpp`
- `include/pyqtgraph/graphicsItems/ViewBox/axisCtrlTemplate_generic.hpp`, `src/pyqtgraph/graphicsItems/ViewBox/axisCtrlTemplate_generic.cpp`
- `src/pyqtgraph/graphicsItems/BarGraphItem.cpp`
- `src/pyqtgraph/graphicsItems/ErrorBarItem.cpp`
- `src/pyqtgraph/graphicsItems/FillBetweenItem.cpp`
- `src/pyqtgraph/graphicsItems/GraphicsLayout.cpp`
- `src/pyqtgraph/graphicsItems/LinearRegionItem.cpp`
- `src/pyqtgraph/graphicsItems/TargetItem.cpp`

## Changed-file ownership

Issue-owned paths:

- graphicsItems manifest coverage entries: `port_manifest.yaml`
- remaining unowned graphicsItems implementation targets under `include/pyqtgraph/graphicsItems/**` and `src/pyqtgraph/graphicsItems/**`
- focused proof: `tests/oracle/test_P4_15_graphicsitems_manifest_coverage.py`
- focused-doc-report: `reports/issues/P4.15/completion.md`

No shared CMake wiring was required for manifest-infra proof; validation is pytest/generator based.

## Failure-mode fixtures tested

`tests/oracle/test_P4_15_graphicsitems_manifest_coverage.py` covers:

- passing graphicsItems 47/47 dashboard count
- deterministic generator output
- stale summary count rejection via `generate_manifest --check`
- missing graphicsItems `status` metadata rejection
- stripped status metadata rejection
