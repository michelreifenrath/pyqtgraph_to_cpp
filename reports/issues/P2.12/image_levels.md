# P2.12 image levels/LUT proof

Issue: #245 / P2.12

## Upstream behavior references

Primary reference: `pyqtgraph-0.14.0` at commit `a20028b98294b9cc8770f2015a92eb342224b788`.

- `pyqtgraph/functions.py:1303-1355`: `rescaleData` applies `(data - offset) * scale`, clips before integer casts, and uses output dtype bounds for integer output.
- `pyqtgraph/functions.py:1358-1380`: `applyLookupTable` clips integer indices to the valid LUT range via `np.take(..., mode='clip')`.
- `pyqtgraph/functions_qimage.py:87-139`: float image levels are required and then rescaled before QImage conversion.
- `pyqtgraph/functions_qimage.py:142-220`: integer image levels and LUTs are combined into effective uint8 data or `Indexed8` color tables.
- `pyqtgraph/functions_qimage.py:222-370`: `try_make_qimage` rejects unsupported levels/LUT/channel combinations and uses pass-through/`Indexed8`/16-bit QImage formats where supported.
- Upstream tests: `tests/test_functions.py:120-138` for rescale dtype clipping and `tests/test_makeARGB.py:221-351` for uint8/uint16/float levels/LUT edge cases.

No optional Qt/OpenCV/NumPy source was fetched; the pinned PyQtGraph source and tests were sufficient.

## Local proof

`tests/core/test_image_levels.cpp` covers:

- rescale clipping to uint8 and explicit clip bounds,
- clipped LUT lookup indices,
- uint8 mono levels via `Format_Indexed8` color-table mapping,
- uint8 levels+inverted LUT effective table mapping,
- uint16 levels+RGB LUT rescaling to `Indexed8`,
- uint16 large grayscale LUT direct application for LUT rows beyond `Indexed8`,
- float mono requiring explicit levels and rescaling to grayscale,
- invalid LUT columns and 4-channel levels returning `std::nullopt`.

## Validation commands

- `cmake --preset dev` — exit 0
- `cmake --build --preset dev` — exit 0
- `ctest --preset dev -L P2.12 --output-on-failure` — exit 0
- `ctest --preset dev -R 'image_levels|functions_qimage|makeQImage' --output-on-failure` — exit 0
- `git diff --check` — exit 0
- `check_pr_scope.py --issue-file ... --changed-files-file ...` — exit 0

`python3 scripts/factory/check_proposed_issues.py --root "$TARGET_REPO_DIR" --source local` was not used as an acceptance gate because that helper lints all Markdown in the product tree and fails on pre-existing non-issue docs/reports. The product repository has no `scripts/check_proposed_issues` executable in this checkout.
