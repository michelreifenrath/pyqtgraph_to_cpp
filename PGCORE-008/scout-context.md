# Code Context

## Files Retrieved
1. `AGENTS.md` (lines 1-41) - repository source-of-truth, owned-file, TDD, upstream-reference, and safety rules.
2. `WORKFLOW.md` (lines 65-80) - validation policy and configured Python gate command source.
3. `docs/pyqtgraph-cpp-port-workflow.md` (lines 310-328, 332-420) - numeric/LUT representation guidance, baseline commands, oracle strategy, and visual classification for LUT generation.
4. `include/pyqtgraph/colormap.hpp` (lines 1-32) - current public `ColorMap` API.
5. `src/pyqtgraph/colormap.cpp` (lines 1-51) - current skeleton implementation and validation behavior.
6. `tests/core/test_ColorMap.cpp` (lines 1-110) - current hand-rolled C++ test style and coverage.
7. `CMakeLists.txt` (lines 43-54, 70-90) - ColorMap source/test target registration; likely issue-scope conflict if a new test file must be built.
8. `oracle/scripts/generate_numeric_oracles.py` (lines 1-22, 227-275, 283-309, 378-420) - current numeric oracle framework; no ColorMap LUT case yet.
9. `reference/source.lock` (lines 1-5) - pinned PyQtGraph reference metadata; local checkout path is declared but absent.
10. `reports/agents/PGCORE-007.md` (lines 1-29) - dependency report; confirms ColorMap is currently skeleton-only and local upstream checkout was absent then too.
11. Pinned upstream `pyqtgraph/colormap.py` fetched read-only from GitHub at commit `a20028b98294b9cc8770f2015a92eb342224b788` (lines 354-416, 546-609, 741-823) - authoritative behavior for map/LUT modes.

## Key Code

Current API is construction/accessors only:

```cpp
// include/pyqtgraph/colormap.hpp:16-29
class ColorMap final {
public:
    ColorMap(std::vector<double> positions, std::vector<QColor> colors, QString name = {});
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] const std::vector<double>& positions() const noexcept;
    [[nodiscard]] const std::vector<QColor>& colors() const noexcept;
    [[nodiscard]] const QString& name() const noexcept;
private:
    std::vector<double> positions_;
    std::vector<QColor> colors_;
    QString name_;
};
```

Current implementation only rejects empty/mismatched stops; it does not sort stops, normalize colors, expose modes, map values, include alpha policy, or generate LUTs (`src/pyqtgraph/colormap.cpp:13-49`). Current tests only assert storage/name/validation (`tests/core/test_ColorMap.cpp:64-97`).

Upstream behavior to port for PGCORE-008:

- Modes/constants: mapping `CLIP=1`, `REPEAT=2`, `MIRROR=3`, `DIVERGING=4`; return `BYTE=1`, `FLOAT=2`, `QCOLOR=3`; string aliases in `enumMap` (upstream `colormap.py:354-373`).
- Constructor sorts stops by position, converts all colors to float RGBA, defaults mapping to CLIP, and allows `pos is None` for equal spacing (upstream `colormap.py:375-416`). C++ skeleton currently preserves order and stores `QColor`; decide whether PGCORE-008 owns sorting/normalization because LUT correctness depends on it.
- `map(data, mode)` gets stops, applies non-CLIP mapping (`repeat`: modulo 1, `diverging`: `data/2+0.5`, `mirror`: abs), then per-channel linear interpolation (`np.interp`) and optionally converts to `QColor` (upstream `colormap.py:546-609`). For CLIP, `np.interp` clips outside stop range to endpoint colors.
- `getStops(mode)` caches/converts colors: BYTE is `(float_rgba * 255).astype(np.ubyte)` (truncate toward zero), FLOAT is 0.0-1.0, QCOLOR uses `QColor.fromRgbF` for float storage (upstream `colormap.py:741-769`).
- `getLookupTable(start=0.0, stop=1.0, nPts=512, alpha=None, mode=BYTE)` builds `np.linspace(start, stop, nPts)`, maps it, and drops alpha for BYTE/FLOAT if `alpha` is false. `alpha=None` means include alpha only if any stop alpha differs from full opacity (`usesAlpha`, upstream `colormap.py:771-823`). QCOLOR always returns colors as QColors.

Likely C++ public API additions in `include/pyqtgraph/colormap.hpp`:

```cpp
enum class ColorMapMapping { Clip = 1, Repeat = 2, Mirror = 3, Diverging = 4 };
enum class ColorMapMode { Byte = 1, Float = 2, QColor = 3 };
// or nested enum/constants to keep PyQtGraph naming close.
[[nodiscard]] bool usesAlpha() const noexcept;
[[nodiscard]] std::vector<QColor> getLookupTable(double start = 0.0, double stop = 1.0,
                                                 std::size_t nPts = 512,
                                                 std::optional<bool> alpha = std::nullopt,
                                                 ColorMapMode mode = ColorMapMode::Byte) const;
```

But if BYTE/FLOAT parity is required, `std::vector<QColor>` alone is insufficient because upstream returns `(nPts,3/4)` byte/float arrays. Consider explicit return types such as `std::vector<std::array<uint8_t, 3/4>>` and `std::vector<std::array<double, 3/4>>`, or a small LUT struct with mode/hasAlpha/channel count. Avoid guessing if the issue has an API expectation not visible in this worktree.

Oracle fixture gap: `oracle/fixtures/numeric/colormap_lut.json` does not exist; `oracle/scripts/generate_numeric_oracles.py` currently generates only `affine_transform` and `log_mapping` (`oracle/scripts/generate_numeric_oracles.py:227-275`). The owned file list includes only the fixture JSON, not the generator script, so either hand-author from a trusted upstream probe or escalate if generator changes are required.

## Architecture

`pyqtgraph_cpp` is a single CMake library. When Qt Core/Gui are found, `src/pyqtgraph/colormap.cpp` is compiled into the library and linked PUBLIC to Qt (`CMakeLists.txt:43-54`). Tests are standalone executables using small local CHECK helpers rather than Qt Test assertions. The existing ColorMap test target is `pyqtgraph_cpp_core_colormap`, registered as CTest name `pyqtgraph_cpp.core.ColorMap` and currently points to `tests/core/test_ColorMap.cpp` (`CMakeLists.txt:81-89`).

PGCORE-008 owned files include a new `tests/core/test_ColorMap_LUT.cpp`, but `CMakeLists.txt` is not owned. Without a CMake edit, the new test will not be built/run by `ctest`; this is a scope conflict to resolve before implementation. The Done request also asks for `reports/agents/PGCORE-008.md`, but reports are not in the owned-file list; note/escalate this conflict.

Reference/oracle flow: `reference/source.lock` points to `reference/pyqtgraph`, but that checkout is absent. Existing oracle tooling can materialize a temporary pinned checkout if invoked, but the current numeric generator has no ColorMap probe. Repository guidance says read pinned upstream when behavior matters and request/write an oracle probe rather than guessing (`AGENTS.md:18-23`).

Visual validation: docs classify LUT generation as visual optional, numeric authoritative (`docs/pyqtgraph-cpp-port-workflow.md:416-420`). If visual artifacts are required by the issue, required layout is under `reports/visual-diffs/<case>/` (`docs/pyqtgraph-cpp-port-workflow.md:422-430`), but reports are outside owned files.

## Start Here

Open `include/pyqtgraph/colormap.hpp` first. The main decision is the C++ LUT return API and mode representation; the header must define that before `src/pyqtgraph/colormap.cpp`, `tests/core/test_ColorMap_LUT.cpp`, and `oracle/fixtures/numeric/colormap_lut.json` can be aligned.

## Constraints, Risks, and Validation

- Edit only owned files unless supervisor/human expands scope: `include/pyqtgraph/colormap.hpp`, `src/pyqtgraph/colormap.cpp`, `tests/core/test_ColorMap_LUT.cpp`, `oracle/fixtures/numeric/colormap_lut.json`.
- Scope conflicts: `CMakeLists.txt` is needed to register `test_ColorMap_LUT.cpp`; `reports/agents/PGCORE-008.md` is requested by Done but not owned.
- Pinned local upstream checkout is absent (`reference/pyqtgraph` missing). Use the pinned commit in `reference/source.lock` for oracle values; do not rely on installed `pyqtgraph` (not installed here).
- Important parity details: stop sorting, endpoint clipping via interpolation, byte truncation after `*255`, default LUT size 512, default alpha auto-detection, and alpha channel removal for non-QCOLOR when alpha is false.
- Validation requested by issue: `scripts/gate focus PGCORE-008` (note current `scripts/gate` exposes modes only, so verify whether `PGCORE-008` is accepted by another wrapper or should be omitted), `scripts/gate commit`, `python3 -m pytest -q`, and `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md`.
