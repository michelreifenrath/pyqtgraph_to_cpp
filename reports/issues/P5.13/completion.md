# P5.13 ColorMapButton/ColorMapMenu completion report

- Issue: GitHub #250 / P5.13
- Validation class: interaction-ui

## Summary

Implemented native Qt/C++ `ColorMapButton` and `ColorMapMenu` with horizontal colormap painting, menu selection, None/user entries, local submenu lazy population, and selection signals.

## Validation

| Command | Exit code | Result |
| --- | ---: | --- |
| `cmake --preset dev` | 0 | pass |
| `cmake --build --preset dev --parallel` | 0 | pass |
| `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.13 --output-on-failure` | 0 | pass |
| `python3 -m pytest -q` | 0 | pass |
| `git diff --check` | 0 | pass |

## Artifacts

- `include/cppqtgraph/widgets/ColorMapButton.hpp`
- `src/cppqtgraph/widgets/ColorMapButton.cpp`
- `include/cppqtgraph/widgets/ColorMapMenu.hpp`
- `src/cppqtgraph/widgets/ColorMapMenu.cpp`
- `tests/widgets/test_ColorMapMenu_P5_13.cpp`
- `reports/visual-diffs/ColorMapMenu/<case>/{reference.png,actual.png,diff.png,metrics.json,gpt5_vision_review.md}`
- `reports/issues/P5.13/*`
