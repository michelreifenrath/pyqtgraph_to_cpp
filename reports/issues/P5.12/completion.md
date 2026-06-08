# P5.12 ColorButton completion report

- Issue: GitHub #249 / P5.12
- Validation class: interaction-ui

## Summary

Implemented native Qt/C++ `ColorButton` with color swatch painting, QColorDialog alpha/non-native options, changing/changed signals, and save/restore RGBA state.

## Validation

| Command | Exit code | Result |
| --- | ---: | --- |
| `cmake --preset dev` | 0 | pass |
| `cmake --build --preset dev --parallel` | 0 | pass |
| `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.12 --output-on-failure` | 0 | pass |
| `python3 -m pytest -q` | 0 | pass |
| `git diff --check` | 0 | pass |

## Artifacts

- `include/pyqtgraph/widgets/ColorButton.hpp`
- `src/pyqtgraph/widgets/ColorButton.cpp`
- `tests/widgets/test_ColorButton_P5_12.cpp`
- `reports/visual-diffs/ColorButton/<case>/{reference.png,actual.png,diff.png,metrics.json,gpt5_vision_review.md}`
- `reports/issues/P5.12/*`
