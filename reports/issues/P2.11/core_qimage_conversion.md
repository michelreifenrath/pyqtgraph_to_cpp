# P2.11 core QImage conversion evidence

## Upstream reference

- PyQtGraph `pyqtgraph-0.14.0` @ `a20028b98294b9cc8770f2015a92eb342224b788`
- Source: `pyqtgraph/functions_qimage.py::try_make_qimage`
- Format oracle: `tests/test_ImageFormat.py` (no levels, no LUT pass-through cases)

## C++ API

`pyqtgraph::tryMakeQImage` overloads mirror upstream unsupported-as-`None` for the narrow pass-through subset:

| Input | Upstream format | C++ result |
| --- | --- | --- |
| `uint8` `(H, W)` | `Format_Grayscale8` | `std::optional<QImage>` with RGB888 channel order N/A |
| `uint8` `(H, W, 3)` | `Format_RGB888` | RGB byte order per channel index 0/1/2 |
| `uint8` `(H, W, 4)` | `Format_RGBA8888` | RGBA byte order, alpha preserved |
| `uint16` `(H, W)` | `Format_Grayscale16` | 16-bit samples when Qt format exists |
| `uint16` `(H, W, 4)` | `Format_RGBA64` | four `uint16` channels per pixel |

Unsupported combinations return `std::nullopt` (e.g. null data, zero extents, 1/2/5+ channels, `uint16` RGB triplets).

All accepted inputs are copied into a contiguous QImage-owned buffer (upstream `numpy.ascontiguousarray` semantics). Legacy `makeQImage` (BGR/BGRA, transpose/copy options) is unchanged.

## Focused fixtures (embedded in `tests/core/test_functions_qimage.cpp`)

- Grayscale `uint8` `2×3`: pixels `(0,0)=10`, `(2,0)=30`, `(0,1)=40`, `(2,1)=60`
- RGB `uint8` `2×2×3`: channel order matches array indices (not legacy BGR)
- RGBA `uint8` `2×2×4`: alpha values `4`, `128`, `0`, `255` at corners
- Strided `uint8` `2×3` with strides `{6,2}`: extracts `1,3,4,6` into contiguous `Grayscale8`
- `uint16` grayscale `2×2`: values `1000..4000`
- `uint16` RGBA64 `2×2×4`: corner sample `(100,200,300,400)` and `(1300,1400,1500,1600)`

## Out of scope for this shard

Levels, LUT / `Indexed8`, float rescaling, transparent locations, `RGBX8888` without alpha requirement, and cupy/numba branches remain upstream-only until later shards.
