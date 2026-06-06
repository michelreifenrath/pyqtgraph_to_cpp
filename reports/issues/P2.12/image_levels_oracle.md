# P2.12 image levels/LUT oracle notes

Issue #245 focused proof for PyQtGraph 0.14.0 image-helper levels and LUT behavior.

Upstream references (pinned commit `a20028b98294b9cc8770f2015a92eb342224b788`):

- `pyqtgraph/functions.py:1303-1355` (`rescaleData`) defines `(data - offset) * scale`, clips before integer casts, and truncates by integer conversion.
- `pyqtgraph/functions.py:1358-1380` (`applyLookupTable`) indexes LUT rows with clipped indices via `np.take(..., mode='clip')`.
- `pyqtgraph/functions_qimage.py:142-220` combines integer image levels and LUTs into effective LUTs, including LUTs with more than 256 rows without narrowing effective row indices to `uint8`.
- `pyqtgraph/functions_qimage.py:222-369` rejects float images without levels, rejects multi-channel levels in the fast path, and creates `Indexed8`, `Grayscale8`, or RGBA-style buffers for supported fast paths.
- `tests/test_makeARGB.py:215-318` covers uint8 levels, LUT-only, LUT+levels, uint16 levels/LUT, and float levels fixtures.

Local focused C++ proof:

- `tests/core/test_image_levels_qimage.cpp::testApplyLookupTableClipsIndices` verifies clipped LUT indexing for negative and too-large indices.
- `testUint8LevelsUseIndexedColorTable` verifies uint8 levels produce an indexed effective grayscale table with clipped edge values.
- `testUint8LutWithoutLevelsUsesClippedEffectiveTable` verifies integer image + LUT without explicit levels keeps direct 256-row row selection for byte data.
- `testLevelsLutMoreThan256RowsDoesNotTruncateEffectiveIndices` verifies a 512-row LUT uses effective row indices 257 and 511 rather than truncating to `uint8`.
- `testUint16LevelsRescaleToGray8` and `testFloatRequiresLevelsAndRescales` verify uint16 and float range behavior, including float-without-levels unsupported.
