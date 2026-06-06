# P2.12 LUT and levels proof

## Upstream references

- `pyqtgraph/functions_qimage.py` (`_combine_levels_and_lut`, `_rescale_and_lookup_float`, `try_make_qimage`) defines the fast QImage levels/LUT behavior.
- `pyqtgraph/functions.py` (`rescaleData`, `applyLookupTable`) defines clipped scaling/casting and clipped LUT indexing.
- `tests/test_ImageFormat.py` covers expected formats for uint8, uint16, and float images with levels and LUTs.
- `tests/test_makeARGB.py` covers exact edge values for levels, LUT sizes, reversed levels, uint8, uint16, and float inputs.

Reference source: `pyqtgraph-0.14.0`, pinned commit `a20028b98294b9cc8770f2015a92eb342224b788`.

## Local focused proof

`tests/core/test_image_levels.cpp` verifies:

- uint8 mono levels use `Format_Indexed8` color tables without rewriting source indexes.
- uint8 levels combine with RGB LUTs in the effective color table.
- uint16 levels rescale to `Format_Grayscale8`.
- uint16 LUT-only data with 65,536 LUT rows indexes rows 255 and 256 distinctly, proving indexes are not truncated to `std::uint8_t` before LUT lookup.
- levels combined with a 1,024-row RGBA LUT reaches row 300 and row 1023.
- float mono data requires levels and supports levels plus LUT.
- unsupported LUT channel counts return `std::nullopt`.

## Commands run

- `cmake --build --preset dev --target pyqtgraph_cpp_core_image_levels` before production edits: failed as expected because the P2.12 API/helpers were missing.
- `cmake --build --preset dev --target pyqtgraph_cpp_core_image_levels`: passed.
- `ctest --preset dev -L P2.12 --output-on-failure`: passed.
- `cmake --build --preset dev`: passed.
- `ctest --preset dev -R 'functions_qimage|image_levels|makeQImage' --output-on-failure`: passed.
- Issue validation chain `cmake --preset dev && cmake --build --preset dev && ctest --preset dev -L P2.12 --output-on-failure && scripts/check_proposed_issues && git diff --check && git diff --name-only origin/main...HEAD`: CMake/build/CTest passed, then exited 127 because `scripts/check_proposed_issues` is not present in this checkout.
- `git diff --check`: passed.
- Factory `check_pr_scope.py` with `changed-files.txt`: passed.
- Factory `scripts/gate commit --dry-run --workflow WORKFLOW.md`: passed.
