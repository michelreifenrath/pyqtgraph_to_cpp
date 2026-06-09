# P5.07 ImageView / RawImageWidget completion report

- Issue: GitHub #164 / P5.07
- Validation class: pixel-image

## Summary

Implemented minimal native Qt/C++ `RawImageWidget` and `ImageView` with deterministic buffer format, dtype, stride/copy, color order, levels/LUT, and copy/view behavior. Added focused CTest proof `P5.07`.

## Pre-implementation proof

Focused test added before implementation; expected failure without production sources.

## Validation

| Command | Exit code | Result |
| --- | ---: | --- |
| `cmake --preset dev` | 0 | pass |
| `cmake --build --preset dev --parallel` | 0 | pass |
| `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.07 --output-on-failure` | 0 | pass |
| `python3 -m pytest -q` | 0 | pass (262 passed, 11 skipped) |
| `git diff --check` | 0 | pass |

## Manifest-expanded target paths

- `include/pyqtgraph/widgets/RawImageWidget.hpp`
- `src/pyqtgraph/widgets/RawImageWidget.cpp`
- `include/pyqtgraph/imageview/ImageView.hpp`
- `src/pyqtgraph/imageview/ImageView.cpp`
- `include/pyqtgraph/imageview/ImageViewTemplate_generic.hpp`
- `src/pyqtgraph/imageview/ImageViewTemplate_generic.cpp`

## Shared wiring / adjunct paths changed

- `CMakeLists.txt`
- `tests/CMakeLists.txt`
- `port_manifest.yaml` (P5.07 class entries updated; broader generated inventory drift is outside this issue)
- `tests/widgets/test_ImageView_RawImageWidget_P5_07.cpp`
- `reports/issues/P5.07/completion.md`
- `reports/issues/P5.07/pixel_image_buffer.json`

## Artifacts

- `reports/issues/P5.07/pixel_image_buffer.json`
