# PGCORE-002 Implementation Report

## Summary
- Implemented `ArrayView<T, Rank = 1>` with fixed-rank shape and element-stride metadata while preserving the existing `ArrayView<T>` rank-1 spelling.
- Added contiguous C-order stride calculation, caller-provided strided layouts, product-based `size()`, zero-extent emptiness, rank-1 strided `operator[]`, multi-dimensional `operator()`/`at()`, and zero-copy positive-step `slice()`.
- Covered rank-1/rank-2 indexing, strided slices, and invalid slice inputs directly in focused C++ tests.

## API decisions
- Strides are stored and reported in elements, not bytes.
- `slice(axis, begin, end, step)` is rank-preserving and half-open over `[begin, end)` with positive `step` only.
- Invalid slice inputs throw deterministically: `std::invalid_argument` for step zero and `std::out_of_range` for invalid axes/ranges.
- Element accessors remain unchecked; validation is limited to slice construction.

## Validation
- `cmake --preset dev` — exit 0.
- `cmake --build --preset dev --parallel` — exit 0.
- `ctest --preset dev --output-on-failure -R pyqtgraph_cpp.core.ArrayView` — exit 0.
- `python3 -m pytest -q` — exit 0, 171 passed.
- `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` — exit 0.
- `scripts/gate focus PGCORE-002` — exit 2; local parser rejected the issue id argument.
- `scripts/gate focus` — exit 0.
- `scripts/gate commit` — exit 0.
- `python3 oracle/scripts/generate_numeric_oracles.py --check --root .` — exit 0, 2 cases verified after removing the unregistered `array_view.json` fixture.

## Deviations / notes
- `src/pyqtgraph/core/ArrayView.cpp` remains unchanged because `ArrayView` is still a header-only template.
- Removed the hand-authored `oracle/fixtures/numeric/array_view.json` fixture during rework because the numeric oracle directory is generator-managed and rejects unregistered JSON files.
- Did not create `PGCORE-002/implementer.md` because the assignment's hard allowed-file list excludes that path.
- No PR was opened from this Pi session because the repo workflow forbids Pi from committing, pushing, or merging; the release automation is expected to open/update the PR after handoff validation.
